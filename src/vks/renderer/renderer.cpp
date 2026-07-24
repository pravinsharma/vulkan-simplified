#include "vks/renderer/renderer.hpp"

#include "vks/backend/vulkan_context.hpp"
#include "vks/backend/vulkan_utils.hpp"
#include "vks/debug.hpp"
#include "vks/material.hpp"
#include "vks/mesh.hpp"
#include "vks/scene_entity.hpp"
#include "vks/shader_compiler/shader_compiler.hpp"
#include "vks/texture.hpp"
#include "vks/types.hpp"

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>

namespace vks::renderer {

namespace {

PipelineKey make_key(const Material& m) {
    PipelineKey k;
    k.vertexShaderPath = m.vertexShader();
    k.fragmentShaderPath = m.fragmentShader();
    k.topology = m.topology();
    k.cullMode = m.cullMode();
    k.blendMode = m.blendMode();
    k.depthTest = m.depthTest();
    k.depthWrite = m.depthWrite();
    return k;
}

constexpr VkPrimitiveTopology to_vk_topology(Topology t) {
    switch (t) {
        case Topology::TriangleList:  return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case Topology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case Topology::LineList:      return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case Topology::PointList:     return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    }
    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

constexpr VkCullModeFlags to_vk_cull(CullMode mode) {
    switch (mode) {
        case CullMode::Off:   return VK_CULL_MODE_NONE;
        case CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
        case CullMode::Back:  return VK_CULL_MODE_BACK_BIT;
    }
    return VK_CULL_MODE_BACK_BIT;
}

}

struct ForwardRenderer::Impl {
    vks::Scene* scene = nullptr;
    backend::VulkanContext* ctx = nullptr;
    std::unique_ptr<PipelineManager> pipelineManager;
    std::unique_ptr<ResourceManager> resourceManager;
    std::vector<MaterialRecord> materials;

    VkDescriptorPool descPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout setLayout = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

    VkFormat depthFormat = VK_FORMAT_UNDEFINED;

    GpuBuffer perFrameUbo;
    VkDescriptorSet perFrameDescriptorSet = VK_NULL_HANDLE;
    std::unordered_map<const Mesh*, GpuMesh> meshCache;
};

ForwardRenderer::ForwardRenderer(backend::VulkanContext* ctx)
    : ctx_(ctx)
    , pimpl(std::make_unique<Impl>())
{
    pimpl->ctx = ctx;
}

ForwardRenderer::~ForwardRenderer() {
    destroy();
}

void ForwardRenderer::setScene(class Scene* scene) {
    pimpl->scene = scene;
}

bool ForwardRenderer::init(const Config& cfg) {
    if (!ctx_ || !ctx_->isValid()) return false;
    auto* dev = ctx_->device();

    pimpl->pipelineManager = std::make_unique<PipelineManager>(*ctx_);
    pimpl->resourceManager = std::make_unique<ResourceManager>(*ctx_);

    pimpl->setLayout = pimpl->pipelineManager->getDescriptorSetLayout();
    pimpl->pipelineLayout = pimpl->pipelineManager->getDefaultPipelineLayout();

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = 64;

    VkDescriptorPoolCreateInfo dpCI{};
    dpCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dpCI.maxSets = 64;
    dpCI.poolSizeCount = 1;
    dpCI.pPoolSizes = &poolSize;
    dpCI.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    if (vkCreateDescriptorPool(dev, &dpCI, nullptr, &pimpl->descPool) != VK_SUCCESS) return false;

    pimpl->perFrameUbo = pimpl->resourceManager->createUniformBuffer(sizeof(Mat4) + 16);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pimpl->descPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &pimpl->setLayout;
    if (vkAllocateDescriptorSets(dev, &allocInfo, &pimpl->perFrameDescriptorSet) != VK_SUCCESS) return false;

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = pimpl->perFrameUbo.buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(Mat4) + 16;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.dstBinding = 0;
    write.dstSet = pimpl->perFrameDescriptorSet;
    write.descriptorCount = 1;
    write.pBufferInfo = &bufferInfo;
    vkUpdateDescriptorSets(dev, 1, &write, 0, nullptr);

    VkExtent2D extent = ctx_->swapchainExtent();
    pimpl->depthFormat = vks::backend::findDepthFormat(ctx_->physicalDevice());

    return true;
}

void ForwardRenderer::destroy() {
    if (!ctx_) return;
    auto* dev = ctx_->device();

    for (auto& m : pimpl->materials) {
        for (auto& tx : m.gpuTextures) pimpl->resourceManager->destroyTexture(tx);
    }
    pimpl->materials.clear();
    pimpl->meshCache.clear();

    pimpl->resourceManager->destroyBuffer(pimpl->perFrameUbo);
    if (pimpl->perFrameDescriptorSet) vkFreeDescriptorSets(dev, pimpl->descPool, 1, &pimpl->perFrameDescriptorSet);
    if (pimpl->descPool) vkDestroyDescriptorPool(dev, pimpl->descPool, nullptr);
}

MaterialHandle ForwardRenderer::registerMaterial(const Material& material) {
    for (MaterialHandle h = 0; h < static_cast<MaterialHandle>(pimpl->materials.size()); ++h) {
        const auto& existing = pimpl->materials[h];
        if (existing.vertexShaderPath == material.vertexShader() &&
            existing.fragmentShaderPath == material.fragmentShader() &&
            existing.topology == material.topology() &&
            existing.cullMode == material.cullMode() &&
            existing.blendMode == material.blendMode() &&
            existing.depthTest == material.depthTest() &&
            existing.depthWrite == material.depthWrite()) {
            return h;
        }
    }

    MaterialRecord record;
    record.vertexShaderPath = material.vertexShader();
    record.fragmentShaderPath = material.fragmentShader();
    record.topology = material.topology();
    record.cullMode = material.cullMode();
    record.blendMode = material.blendMode();
    record.depthTest = material.depthTest();
    record.depthWrite = material.depthWrite();

    PipelineKey key = make_key(material);
    record.gpuPipeline = pimpl->pipelineManager->getOrCreatePipeline(key);

    for (const auto& [name, tex] : material.textures()) {
        (void)name;
        record.gpuTextures.push_back(pimpl->resourceManager->uploadTexture(*tex));
    }

    MaterialHandle handle = static_cast<MaterialHandle>(pimpl->materials.size());
    pimpl->materials.push_back(record);
    return handle;
}

void ForwardRenderer::unregisterMaterial(MaterialHandle handle) {
    if (handle < pimpl->materials.size()) {
        auto& m = pimpl->materials[handle];
        if (m.gpuPipeline && ctx_) {
            vkDestroyPipeline(ctx_->device(), m.gpuPipeline, nullptr);
        }
        for (auto& tx : m.gpuTextures) pimpl->resourceManager->destroyTexture(tx);
        m = MaterialRecord{};
    }
}

void ForwardRenderer::submitFrame(const FrameSubmitData& frameData) {
    if (!ctx_ || !ctx_->isValid() || !frameData.scene) return;

    auto* dev = ctx_->device();
    VkExtent2D extent = ctx_->swapchainExtent();

    VkCommandBuffer cb = ctx_->beginFrame();
    if (cb == VK_NULL_HANDLE) return;

    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = ctx_->depthImage();
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    VkClearValue clearValues[2]{};
    clearValues[0].color = VkClearColorValue{{frameData.clearColor[0], frameData.clearColor[1], frameData.clearColor[2], frameData.clearColor[3]}};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo rpBI{};
    rpBI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBI.renderPass = ctx_->renderPass();
    rpBI.framebuffer = ctx_->currentFramebuffer();
    rpBI.renderArea.offset = {0, 0};
    rpBI.renderArea.extent = extent;
    rpBI.clearValueCount = 2;
    rpBI.pClearValues = clearValues;

    vkCmdBeginRenderPass(cb, &rpBI, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport vp{};
    vp.width = static_cast<float>(extent.width);
    vp.height = static_cast<float>(extent.height);
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;
    vkCmdSetViewport(cb, 0, 1, &vp);

    VkRect2D scissor{};
    scissor.extent = extent;
    vkCmdSetScissor(cb, 0, 1, &scissor);

    void* mapped = nullptr;
    if (pimpl->perFrameUbo.buffer) {
        vkMapMemory(dev, pimpl->perFrameUbo.memory, 0, sizeof(Mat4) + sizeof(Vec3), 0, &mapped);
        if (mapped) {
            memcpy(mapped, &frameData.viewProjection, sizeof(Mat4));
            memcpy(static_cast<char*>(mapped) + sizeof(Mat4), &frameData.viewPosition, sizeof(Vec3));
            vkUnmapMemory(dev, pimpl->perFrameUbo.memory);
        }
    }

    for (const auto& rec : frameData.drawRecords) {
        if (rec.entity == vks::InvalidEntity || !frameData.scene) continue;
        const auto& entity = frameData.scene->get(rec.entity);
        const auto& mesh = entity.mesh();
        const auto& material = entity.material();

        MaterialRecord matRecord;
        MaterialHandle handle = registerMaterial(material);
        if (handle < pimpl->materials.size()) {
            matRecord = pimpl->materials[handle];
        }

        if (vks::DebugLayer::instance().isEnabled()) {
            if (matRecord.gpuPipeline == VK_NULL_HANDLE) {
                vks::DebugLayer::instance().pushWarning("Null pipeline for material: " + material.vertexShader());
            }
        }

        const Mesh* cpuMesh = &mesh;
        auto mit = pimpl->meshCache.find(cpuMesh);
        GpuMesh gpuMesh{};
        if (mit != pimpl->meshCache.end()) {
            gpuMesh = mit->second;
        } else {
            gpuMesh = pimpl->resourceManager->uploadMesh(mesh);
            pimpl->meshCache[cpuMesh] = gpuMesh;
        }

        vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, matRecord.gpuPipeline);
        vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pimpl->pipelineLayout, 0, 1, &pimpl->perFrameDescriptorSet, 0, nullptr);
        vkCmdPushConstants(cb, pimpl->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Mat4), &rec.modelMatrix);

        Vec4 tint = rec.tintColor.value_or(vks::Color(1.0f, 1.0f, 1.0f, 1.0f));
        float roughness = rec.roughnessOverride.value_or(-1.0f);
        float metallic = rec.metallicOverride.value_or(-1.0f);
        vkCmdPushConstants(cb, pimpl->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Mat4), sizeof(Vec4) + sizeof(float) * 2, &tint);

        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cb, 0, 1, &gpuMesh.vertexBuffer.buffer, offsets);
        if (gpuMesh.indexCount > 0) {
            vkCmdBindIndexBuffer(cb, gpuMesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(cb, gpuMesh.indexCount, 1, 0, 0, 0);
        } else {
            vkCmdDraw(cb, gpuMesh.vertexCount, 1, 0, 0);
        }
    }

    if (vks::DebugLayer::instance().isEnabled()) {
        vks::FrameDiagnostics diag;
        diag.drawCalls = static_cast<uint32_t>(frameData.drawRecords.size());
        diag.pipelineCount = static_cast<uint32_t>(pimpl->materials.size());
        for (const auto& rec : frameData.drawRecords) {
            if (rec.entity != vks::InvalidEntity && frameData.scene) {
                const auto& entity = frameData.scene->get(rec.entity);
                const auto& mesh = entity.mesh();
                diag.triangleCount += mesh.indexCount() > 0 ? mesh.indexCount() / 3 : mesh.vertexCount() / 3;
            }
        }
        vks::DebugLayer::instance().setFrameDiagnostics(diag);
    }

    vkCmdEndRenderPass(cb);

    ctx_->endFrame();
    ctx_->present();
}

void ForwardRenderer::onResize(uint32_t width, uint32_t height) {
    if (!ctx_ || !ctx_->isValid()) return;
    ctx_->recreateSwapchain();
}

}
