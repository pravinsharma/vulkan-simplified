#include "vulkan_simplified/render_graph.hpp"

#include "vulkan_simplified/backend/vulkan_context.hpp"
#include "vulkan_simplified/backend/vulkan_utils.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.h>

namespace vks::renderer {

namespace {

struct RGTexture {
    VkImage image = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkImageUsageFlags usage = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    VkClearValue clearValue{{0.0f, 0.0f, 0.0f, 0.0f}};
    bool external = false;
};

struct RGBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceSize size = 0;
    VkBufferUsageFlags usage = 0;
};

struct RGPass {
    std::unique_ptr<RenderGraph::Pass> pass;
    std::vector<RenderGraph::Resource> inputs;
    std::vector<RenderGraph::Resource> outputs;
};

}

struct RenderGraph::Impl {
    backend::VulkanContext* ctx = nullptr;
    uint32_t width;
    uint32_t height;
    std::vector<RGTexture> textures;
    std::vector<RGBuffer> buffers;
    std::vector<RGPass> passes;
    std::vector<VkImageView> externalViews;
    std::vector<VkImage> externalImages;
    std::vector<VkFormat> externalFormats;
};

RenderGraph::RenderGraph(backend::VulkanContext& ctx, uint32_t width, uint32_t height)
    : ctx_(ctx)
    , pimpl(std::make_unique<Impl>())
{
    pimpl->ctx = &ctx;
    pimpl->width = width;
    pimpl->height = height;
}

RenderGraph::~RenderGraph() = default;

RenderGraph::Resource RenderGraph::addTexture(const TextureDesc& desc) {
    RGTexture tex{};
    tex.width = desc.width;
    tex.height = desc.height;
    tex.format = desc.format;
    tex.usage = desc.usage;
    tex.clearValue = desc.clearValue;

    VkImageCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ci.imageType = VK_IMAGE_TYPE_2D;
    ci.format = tex.format;
    ci.extent = {tex.width, tex.height, 1};
    ci.mipLevels = 1;
    ci.arrayLayers = 1;
    ci.samples = VK_SAMPLE_COUNT_1_BIT;
    ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    ci.usage = tex.usage;
    ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkDevice dev = ctx_.device();
    if (vkCreateImage(dev, &ci, nullptr, &tex.image) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render graph texture");
    }

    VkMemoryRequirements reqs;
    vkGetImageMemoryRequirements(dev, tex.image, &reqs);

    VkMemoryAllocateInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize = reqs.size;
    ai.memoryTypeIndex = vks::backend::findMemoryType(
        ctx_.physicalDevice(), reqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(dev, &ai, nullptr, &tex.memory) != VK_SUCCESS) {
        vkDestroyImage(dev, tex.image, nullptr);
        throw std::runtime_error("Failed to allocate render graph texture memory");
    }
    vkBindImageMemory(dev, tex.image, tex.memory, 0);

    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    if (tex.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    tex.view = ctx_.createImageView(tex.image, tex.format, aspect, 1);

    pimpl->textures.push_back(tex);
    return Resource::texture(static_cast<uint32_t>(pimpl->textures.size() - 1));
}

RenderGraph::Resource RenderGraph::addBuffer(const BufferDesc& desc) {
    RGBuffer buf{};
    buf.size = desc.size;
    buf.usage = desc.usage;

    VkDevice dev = ctx_.device();
    VkBufferCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    ci.size = buf.size;
    ci.usage = buf.usage;
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(dev, &ci, nullptr, &buf.buffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render graph buffer");
    }

    VkMemoryRequirements reqs;
    vkGetBufferMemoryRequirements(dev, buf.buffer, &reqs);

    VkMemoryAllocateInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize = reqs.size;
    ai.memoryTypeIndex = vks::backend::findMemoryType(
        ctx_.physicalDevice(), reqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(dev, &ai, nullptr, &buf.memory) != VK_SUCCESS) {
        vkDestroyBuffer(dev, buf.buffer, nullptr);
        throw std::runtime_error("Failed to allocate render graph buffer memory");
    }
    vkBindBufferMemory(dev, buf.buffer, buf.memory, 0);

    pimpl->buffers.push_back(buf);
    return Resource::buffer(static_cast<uint32_t>(pimpl->buffers.size() - 1));
}

void RenderGraph::addExternalTexture(VkImage image, VkImageView view, VkFormat format) {
    RGTexture tex{};
    tex.image = image;
    tex.view = view;
    tex.format = format;
    tex.external = true;
    pimpl->textures.push_back(tex);
    pimpl->externalImages.push_back(image);
    pimpl->externalViews.push_back(view);
    pimpl->externalFormats.push_back(format);
}

void RenderGraph::addPass(std::unique_ptr<Pass> pass) {
    RGPass rp;
    rp.pass = std::move(pass);
    rp.inputs = std::vector<Resource>(pass->inputs().begin(), pass->inputs().end());
    rp.outputs = std::vector<Resource>(pass->outputs().begin(), pass->outputs().end());
    pimpl->passes.push_back(std::move(rp));
}

bool RenderGraph::compile() {
    return !pimpl->passes.empty();
}

void RenderGraph::execute(VkCommandBuffer cmd, uint32_t frameIndex) {
    (void)frameIndex;
    for (const auto& rp : pimpl->passes) {
        if (rp.pass) {
            RenderGraphContext ctx(*this, cmd);
            rp.pass->execute(cmd, ctx);
        }
    }
}

void RenderGraph::onResize(uint32_t width, uint32_t height) {
    pimpl->width = width;
    pimpl->height = height;
}

VkImageView RenderGraph::getTextureView(Resource res) const {
    if (res.type() != Resource::Type::Texture) return VK_NULL_HANDLE;
    if (res.index() >= pimpl->textures.size()) return VK_NULL_HANDLE;
    return pimpl->textures[res.index()].view;
}

VkImage RenderGraph::getTextureImage(Resource res) const {
    if (res.type() != Resource::Type::Texture) return VK_NULL_HANDLE;
    if (res.index() >= pimpl->textures.size()) return VK_NULL_HANDLE;
    return pimpl->textures[res.index()].image;
}

VkFormat RenderGraph::getTextureFormat(Resource res) const {
    if (res.type() != Resource::Type::Texture) return VK_FORMAT_UNDEFINED;
    if (res.index() >= pimpl->textures.size()) return VK_FORMAT_UNDEFINED;
    return pimpl->textures[res.index()].format;
}

VkExtent2D RenderGraph::extent() const {
    return {pimpl->width, pimpl->height};
}

}
