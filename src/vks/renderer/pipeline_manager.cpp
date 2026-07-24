#include "vks/renderer/pipeline_manager.hpp"

#include "vks/backend/vulkan_context.hpp"
#include "vks/shader_compiler/shader_compiler.hpp"
#include "vks/types.hpp"
#include "vks/mesh.hpp"

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

#include <vulkan/vulkan.h>

namespace vks::renderer {

namespace {

constexpr VkFrontFace to_vk_front_face(CullMode mode) {
    return VK_FRONT_FACE_COUNTER_CLOCKWISE;
}

constexpr VkCullModeFlags to_vk_cull(CullMode mode) {
    switch (mode) {
        case CullMode::Off:   return VK_CULL_MODE_NONE;
        case CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
        case CullMode::Back:  return VK_CULL_MODE_BACK_BIT;
    }
    return VK_CULL_MODE_BACK_BIT;
}

constexpr VkPrimitiveTopology to_vk_topology(Topology topo) {
    switch (topo) {
        case Topology::TriangleList:  return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case Topology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case Topology::LineList:      return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case Topology::PointList:     return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    }
    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

}

struct PipelineManager::Impl {
    explicit Impl(backend::VulkanContext& c) : ctx(c) {}
    backend::VulkanContext& ctx;
    VkDescriptorSetLayout setLayout = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    std::unordered_map<PipelineKey, VkPipeline, PipelineKeyHash> pipelineCache;
};

PipelineManager::PipelineManager(backend::VulkanContext& ctx)
    : pimpl(std::make_unique<Impl>(ctx))
{
}

PipelineManager::~PipelineManager() {
    if (!pimpl) return;
    auto& ctx = pimpl->ctx;
    for (auto& [key, pipeline] : pimpl->pipelineCache) {
        if (pipeline) vkDestroyPipeline(ctx.device(), pipeline, nullptr);
    }
    if (pimpl->pipelineLayout) vkDestroyPipelineLayout(ctx.device(), pimpl->pipelineLayout, nullptr);
    if (pimpl->setLayout) vkDestroyDescriptorSetLayout(ctx.device(), pimpl->setLayout, nullptr);
}

VkPipelineCache PipelineManager::getCache() const {
    return pimpl->ctx.pipelineCache();
}

VkDescriptorSetLayout PipelineManager::getDescriptorSetLayout() const {
    if (!pimpl->setLayout) {
        const_cast<PipelineManager*>(this)->createDefaultLayouts();
    }
    return pimpl->setLayout;
}

VkPipelineLayout PipelineManager::getPipelineLayout() const {
    if (!pimpl->pipelineLayout) {
        const_cast<PipelineManager*>(this)->createDefaultLayouts();
    }
    return pimpl->pipelineLayout;
}

VkPipelineLayout PipelineManager::getDefaultPipelineLayout() const {
    return getPipelineLayout();
}

VkPipeline PipelineManager::getOrCreatePipeline(const PipelineKey& key) {
    auto it = pimpl->pipelineCache.find(key);
    if (it != pimpl->pipelineCache.end()) {
        return it->second;
    }

    auto vertSpirv = vks::compileShader(key.vertexShaderPath);
    auto fragSpirv = vks::compileShader(key.fragmentShaderPath);

    VkShaderModule vertModule = pimpl->ctx.createShaderModule(vertSpirv);
    VkShaderModule fragModule = pimpl->ctx.createShaderModule(fragSpirv);

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertModule;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragModule;
    stages[1].pName = "main";

    VkVertexInputAttributeDescription attrs[3]{};
    attrs[0].binding = 0;
    attrs[0].location = 0;
    attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[0].offset = offsetof(Vertex, position);

    attrs[1].binding = 0;
    attrs[1].location = 1;
    attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[1].offset = offsetof(vks::Vertex, normal);

    attrs[2].binding = 0;
    attrs[2].location = 2;
    attrs[2].format = VK_FORMAT_R32G32_SFLOAT;
    attrs[2].offset = offsetof(vks::Vertex, uv);

    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(vks::Vertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkPipelineVertexInputStateCreateInfo vi{};
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vi.vertexBindingDescriptionCount = 1;
    vi.pVertexBindingDescriptions = &binding;
    vi.vertexAttributeDescriptionCount = 3;
    vi.pVertexAttributeDescriptions = attrs;

    VkPipeline pipeline = pimpl->ctx.createGraphicsPipeline(
        stages, 2,
        &vi,
        getDefaultPipelineLayout(),
        pimpl->ctx.renderPass(),
        to_vk_topology(key.topology),
        key.depthTest, key.depthWrite,
        to_vk_cull(key.cullMode),
        key.blendMode != vks::BlendMode::Off);

    vkDestroyShaderModule(pimpl->ctx.device(), vertModule, nullptr);
    vkDestroyShaderModule(pimpl->ctx.device(), fragModule, nullptr);

    pimpl->pipelineCache[key] = pipeline;
    return pipeline;
}

void PipelineManager::createDefaultLayouts() {
    VkDescriptorSetLayoutBinding uboBinding{};
    uboBinding.binding = 0;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo dslCI{};
    dslCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dslCI.bindingCount = 1;
    dslCI.pBindings = &uboBinding;

    vkCreateDescriptorSetLayout(pimpl->ctx.device(), &dslCI, nullptr, &pimpl->setLayout);

    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(Mat4) + sizeof(Vec4) + sizeof(float) * 2;

    VkPipelineLayoutCreateInfo plCI{};
    plCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plCI.setLayoutCount = 1;
    plCI.pSetLayouts = &pimpl->setLayout;
    plCI.pushConstantRangeCount = 1;
    plCI.pPushConstantRanges = &pushRange;

    vkCreatePipelineLayout(pimpl->ctx.device(), &plCI, nullptr, &pimpl->pipelineLayout);
}

}
