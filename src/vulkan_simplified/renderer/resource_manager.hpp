#pragma once

#include <cstdint>
#include <deque>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "vulkan_simplified/backend/vulkan_context.hpp"
#include "vulkan_simplified/material.hpp"
#include "vulkan_simplified/mesh.hpp"
#include "vulkan_simplified/texture.hpp"

namespace vks::renderer {

class ResourceManager;

struct GpuTexture {
    VkImage image = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    uint32_t mipLevels = 1;
};

struct GpuBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceSize size = 0;
    VkDeviceSize offset = 0;
};

struct GpuMesh {
    GpuBuffer vertexBuffer;
    GpuBuffer indexBuffer;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
};

class ResourceManager {
public:
    explicit ResourceManager(backend::VulkanContext& ctx);
    ~ResourceManager();

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    GpuTexture uploadTexture(const class Texture& texture);
    void destroyTexture(GpuTexture& tex);

    GpuMesh uploadMesh(const class Mesh& mesh);

    GpuBuffer createUniformBuffer(VkDeviceSize size);
    void destroyBuffer(GpuBuffer& buf);

    void destroyFrameData();

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

}
