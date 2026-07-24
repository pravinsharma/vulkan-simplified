#include "vks/renderer/resource_manager.hpp"

#include "vks/backend/vulkan_context.hpp"
#include "vks/material.hpp"
#include "vks/mesh.hpp"
#include "vks/texture.hpp"
#include "vks/types.hpp"

#include <algorithm>
#include <bit>
#include <cstring>
#include <vector>
#include <vulkan/vulkan.h>

namespace vks::renderer {

struct ResourceManager::Impl {
    explicit Impl(backend::VulkanContext& c) : ctx(c) {}
    backend::VulkanContext& ctx;
    std::vector<GpuTexture> ownedTextures;
    std::vector<GpuBuffer> ownedBuffers;
    std::vector<GpuMesh> ownedMeshes;
};

ResourceManager::ResourceManager(backend::VulkanContext& ctx)
    : pimpl(std::make_unique<Impl>(ctx))
{
}

ResourceManager::~ResourceManager() {
    if (!pimpl) return;
    auto* dev = pimpl->ctx.device();
    for (auto& tex : pimpl->ownedTextures) {
        if (tex.sampler)   vkDestroySampler(dev, tex.sampler, nullptr);
        if (tex.imageView) vkDestroyImageView(dev, tex.imageView, nullptr);
        if (tex.image)     vkDestroyImage(dev, tex.image, nullptr);
        if (tex.memory)    vkFreeMemory(dev, tex.memory, nullptr);
    }
    for (auto& mesh : pimpl->ownedMeshes) {
        if (mesh.vertexBuffer.buffer) vkDestroyBuffer(dev, mesh.vertexBuffer.buffer, nullptr);
        if (mesh.vertexBuffer.memory) vkFreeMemory(dev, mesh.vertexBuffer.memory, nullptr);
        if (mesh.indexBuffer.buffer) vkDestroyBuffer(dev, mesh.indexBuffer.buffer, nullptr);
        if (mesh.indexBuffer.memory) vkFreeMemory(dev, mesh.indexBuffer.memory, nullptr);
    }
    destroyFrameData();
}

GpuTexture ResourceManager::uploadTexture(const class Texture& texture) {
    auto& ctx = pimpl->ctx;
    Impl* impl = pimpl.get();
    (void)impl;

    VkFormat fmt = VK_FORMAT_R8G8B8A8_UNORM;
    if (texture.format() == TextureFormat::BC1) fmt = VK_FORMAT_BC1_RGB_UNORM_BLOCK;
    else if (texture.format() == TextureFormat::BC3) fmt = VK_FORMAT_BC3_UNORM_BLOCK;
    else if (texture.format() == TextureFormat::R16G16B16A16SFloat) fmt = VK_FORMAT_R16G16B16A16_SFLOAT;

    uint32_t mipLevels = texture.hasMips() ? (uint32_t)1 + (uint32_t)std::bit_width(
        std::max(texture.width(), texture.height() > 1u ? texture.height() : 1u) - 1u) : 1;
    mipLevels = std::clamp(mipLevels, 1u, 12u);

    VkDeviceMemory mem = VK_NULL_HANDLE;
    VkImage image = ctx.createImage(texture.width(), texture.height(), mipLevels, fmt,
                                    VK_IMAGE_TILING_OPTIMAL,
                                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                        VK_IMAGE_USAGE_SAMPLED_BIT,
                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mem);

    VkImageView imageView = ctx.createImageView(image, fmt, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
    VkSampler sampler = ctx.createSampler();

    GpuTexture gpu{};
    gpu.image = image;
    gpu.imageView = imageView;
    gpu.memory = mem;
    gpu.sampler = sampler;
    gpu.mipLevels = mipLevels;

    impl->ownedTextures.push_back(gpu);
    return gpu;
}

void ResourceManager::destroyTexture(GpuTexture& tex) {
    auto& ctx = pimpl->ctx;
    if (tex.sampler)   vkDestroySampler(ctx.device(), tex.sampler, nullptr);
    if (tex.imageView) vkDestroyImageView(ctx.device(), tex.imageView, nullptr);
    if (tex.image)     vkDestroyImage(ctx.device(), tex.image, nullptr);
    if (tex.memory)    vkFreeMemory(ctx.device(), tex.memory, nullptr);
}

GpuMesh ResourceManager::uploadMesh(const class Mesh& mesh) {
    auto verts  = mesh.vertices();
    auto indices = mesh.indices();
    uint32_t vCount = mesh.vertexCount();
    uint32_t iCount = mesh.indexCount();

    auto& ctx = pimpl->ctx;
    GpuMesh gpu{};
    gpu.vertexCount = vCount;
    gpu.indexCount = iCount;

    VkDeviceSize vSize = static_cast<VkDeviceSize>(verts.size_bytes());
    if (vSize > 0) {
        VkDeviceMemory vMem = VK_NULL_HANDLE;
        gpu.vertexBuffer.buffer = ctx.createBuffer(vSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vMem);
        gpu.vertexBuffer.memory = vMem;
        gpu.vertexBuffer.size = vSize;
        void* mapped = ctx.mapBuffer(gpu.vertexBuffer.buffer, vMem, vSize);
        memcpy(mapped, verts.data(), vSize);
        ctx.unmapBuffer(vMem);
    }

    VkDeviceSize iSize = static_cast<VkDeviceSize>(indices.size_bytes());
    if (iSize > 0) {
        VkDeviceMemory iMem = VK_NULL_HANDLE;
        gpu.indexBuffer.buffer = ctx.createBuffer(iSize,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, iMem);
        gpu.indexBuffer.memory = iMem;
        gpu.indexBuffer.size = iSize;
        void* mapped = ctx.mapBuffer(gpu.indexBuffer.buffer, iMem, iSize);
        memcpy(mapped, indices.data(), iSize);
        ctx.unmapBuffer(iMem);
    }

    pimpl->ownedMeshes.push_back(gpu);
    return gpu;
}

GpuBuffer ResourceManager::createUniformBuffer(VkDeviceSize size) {
    auto& ctx = pimpl->ctx;
    VkDeviceMemory mem = VK_NULL_HANDLE;
    VkBuffer buf = ctx.createBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, mem);
    GpuBuffer gb{};
    gb.buffer = buf;
    gb.memory = mem;
    gb.size = size;
    gb.offset = 0;
    pimpl->ownedBuffers.push_back(gb);
    return gb;
}

void ResourceManager::destroyBuffer(GpuBuffer& buf) {
    if (!pimpl) return;
    auto& ctx = pimpl->ctx;
    if (buf.buffer) {
        vkDestroyBuffer(ctx.device(), buf.buffer, nullptr);
        buf.buffer = VK_NULL_HANDLE;
    }
    if (buf.memory) {
        vkFreeMemory(ctx.device(), buf.memory, nullptr);
        buf.memory = VK_NULL_HANDLE;
    }
    buf.size = 0;
    buf.offset = 0;
}

void ResourceManager::destroyFrameData() {
    Impl* impl = pimpl.get();
    if (!impl) return;
    for (auto& buf : impl->ownedBuffers) {
        if (buf.buffer) {
            vkDestroyBuffer(impl->ctx.device(), buf.buffer, nullptr);
            vkFreeMemory(impl->ctx.device(), buf.memory, nullptr);
        }
    }
    impl->ownedBuffers.clear();
}

}
