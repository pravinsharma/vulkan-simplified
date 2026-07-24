#pragma once

#include <vulkan/vulkan.h>

#include <optional>

namespace vks::backend {

struct QueueFamilyIndices {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;
    std::optional<uint32_t> transfer;

    bool isComplete() const {
        return graphics.has_value() && present.has_value();
    }
};

struct SwapchainImage {
    VkImage image = VK_NULL_HANDLE;
    VkImageView image_view = VK_NULL_HANDLE;
    uint32_t index = 0;
};

inline constexpr const char* REQUIRED_DEVICE_EXTENSIONS[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
};
inline constexpr uint32_t REQUIRED_DEVICE_EXTENSIONS_COUNT =
    static_cast<uint32_t>(std::size(REQUIRED_DEVICE_EXTENSIONS));

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
bool isVulkan13Supported(VkPhysicalDevice device);
VkSurfaceFormatKHR pickSurfaceFormat(VkPhysicalDevice device, VkSurfaceKHR surface);
VkPresentModeKHR pickPresentMode(VkPhysicalDevice device, VkSurfaceKHR surface, bool vsync);
VkExtent2D pickSwapExtent(const VkSurfaceCapabilitiesKHR& caps, int width, int height);
VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect);
uint32_t findMemoryType(VkPhysicalDevice phys, uint32_t type_filter, VkMemoryPropertyFlags props);
VkFence createFence(VkDevice device, bool signaled);
VkCommandPool createCommandPool(VkDevice device, uint32_t queue_family, VkCommandPoolCreateFlags flags = 0);

VkFormat findDepthFormat(VkPhysicalDevice phys);

}
