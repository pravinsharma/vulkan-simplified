#pragma once

#include "sdl_window.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <deque>
#include <functional>
#include <span>
#include <vector>

namespace vks::backend {

class VulkanContext {
public:
    using PresentCallback = std::function<bool(uint32_t /*imageIndex*/)>;

    struct SwapchainImageInfo {
        VkImage image = VK_NULL_HANDLE;
        VkImageView image_view = VK_NULL_HANDLE;
        uint32_t index = 0;
    };

    VulkanContext(SdlWindow& window, bool headless, bool enable_validation);
    ~VulkanContext();

    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;
    VulkanContext(VulkanContext&&) noexcept;
    VulkanContext& operator=(VulkanContext&&) noexcept;

    bool recreateSwapchain();
    VkResult acquireNextImage(uint64_t timeout_ns, uint32_t* image_index);
    bool present(uint32_t image_index);
    bool present();

    bool isValid() const;
    VkFormat swapchainImageFormat() const;
    VkExtent2D swapchainExtent() const;
    bool isHeadless() const;

    VkInstance instance() const;
    VkDevice device() const;
    VkPhysicalDevice physicalDevice() const;
    VkQueue graphicsQueue() const;
    VkQueue presentQueue() const;
    uint32_t graphicsFamily() const;
    uint32_t presentFamily() const;

    VkCommandPool graphicsCommandPool() const;
    VkCommandPool transferCommandPool() const;

    uint32_t currentFrame() const;
    VkCommandBuffer beginFrame();
    VkCommandBuffer currentCommandBuffer() const;
    void endFrame();
    VkFramebuffer currentFramebuffer() const;
    void updateDescriptorSets(std::span<const VkWriteDescriptorSet> writes);
    void updateDescriptorSets(std::span<VkWriteDescriptorSet> writes);

    VkPipelineCache pipelineCache() const;
    VkImage depthImage() const;
    VkImageView depthImageView() const;
    VkDescriptorSetLayout createDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo& createInfo);
    VkDescriptorSetLayout createDescriptorSetLayout(const VkDescriptorSetLayoutBinding* bindings, uint32_t bindingCount);
    VkPipelineLayout createPipelineLayout(const VkDescriptorSetLayout* setLayouts, uint32_t setCount, const VkPushConstantRange* pushRanges, uint32_t pushCount);
    VkPipelineLayout createPipelineLayout(const VkDescriptorSetLayout& setLayout);
    VkShaderModule createShaderModule(std::span<const uint32_t> spirv);
    VkPipeline createGraphicsPipeline(const VkPipelineShaderStageCreateInfo* stages, uint32_t stageCount,
                                      const VkPipelineVertexInputStateCreateInfo* vertexInput,
                                      VkPipelineLayout layout, VkRenderPass renderPass,
                                      VkPrimitiveTopology topology, bool depthTest, bool depthWrite,
                                      VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT,
                                      bool blendEnable = false);
    VkRenderPass renderPass() const;

    VkImage createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format,
                        VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags props, VkDeviceMemory& outMemory);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect, uint32_t mipLevels);
    VkSampler createSampler();
    VkBuffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props, VkDeviceMemory& outMemory);
    void* mapBuffer(VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize size);
    void unmapBuffer(VkDeviceMemory memory);

    static VkFormat findDepthFormat(VkPhysicalDevice phys);

    struct Impl;
    Impl* getImpl() const { return impl_.get(); }

private:
    void createDepthResources();
    void createFramebuffers();

private:
    std::unique_ptr<Impl> impl_;
};

}
