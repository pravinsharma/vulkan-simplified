#include "vks/backend/vulkan_utils.hpp"

#include <vulkan/vulkan.h>

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace vks::backend {

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
    QueueFamilyIndices indices{};

    uint32_t family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, nullptr);

    std::vector<VkQueueFamilyProperties> families(family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, families.data());

    for (uint32_t i = 0; i < family_count; ++i) {
        const auto& props = families[i];

        if (props.queueCount == 0) continue;

        if (props.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics = i;
        }
        if ((props.queueFlags & VK_QUEUE_TRANSFER_BIT) && !indices.transfer.has_value()) {
            indices.transfer = i;
        }
        if (indices.transfer.has_value() && indices.transfer.value() == indices.graphics.value()) {
            indices.transfer = std::nullopt;
        }

        VkBool32 present_support = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);

        if (present_support == VK_TRUE) {
            indices.present = i;
        }

        if (indices.isComplete()) break;
    }

    if (!indices.transfer.has_value()) {
        indices.transfer = indices.graphics;
    }

    return indices;
}

bool isVulkan13Supported(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(device, &props);

    if (VK_API_VERSION_MAJOR(props.apiVersion) < 1 ||
        VK_API_VERSION_MINOR(props.apiVersion) < 3) {
        return false;
    }

    VkPhysicalDeviceFeatures2 features{};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

    VkPhysicalDeviceSynchronization2Features sync2{};
    sync2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    sync2.pNext = features.pNext;
    features.pNext = &sync2;

    VkPhysicalDeviceDynamicRenderingFeatures dr{};
    dr.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dr.pNext = sync2.pNext;
    sync2.pNext = &dr;

    vkGetPhysicalDeviceFeatures2(device, &features);

    return sync2.synchronization2 == VK_TRUE && dr.dynamicRendering == VK_TRUE;
}

VkSurfaceFormatKHR pickSurfaceFormat(VkPhysicalDevice device, VkSurfaceKHR surface) {
    uint32_t count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, nullptr);
    if (count == 0) throw std::runtime_error("No surface formats available");

    std::vector<VkSurfaceFormatKHR> formats(count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, formats.data());

    if (count == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
        return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    }

    for (const auto& fmt : formats) {
        if (fmt.format == VK_FORMAT_R8G8B8A8_UNORM && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return fmt;
        }
    }
    return formats[0];
}

VkPresentModeKHR pickPresentMode(VkPhysicalDevice device, VkSurfaceKHR surface, bool vsync) {
    uint32_t count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, nullptr);
    if (count == 0) return VK_PRESENT_MODE_FIFO_KHR;

    std::vector<VkPresentModeKHR> modes(count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, modes.data());

    if (!vsync) {
        auto mailbox = std::find(modes.begin(), modes.end(), VK_PRESENT_MODE_MAILBOX_KHR);
        if (mailbox != modes.end()) return VK_PRESENT_MODE_MAILBOX_KHR;

        auto immediate = std::find(modes.begin(), modes.end(), VK_PRESENT_MODE_IMMEDIATE_KHR);
        if (immediate != modes.end()) return VK_PRESENT_MODE_IMMEDIATE_KHR;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D pickSwapExtent(const VkSurfaceCapabilitiesKHR& caps, int width, int height) {
    if (caps.currentExtent.width != 0xFFFFFFFF) {
        return caps.currentExtent;
    }

    VkExtent2D extent{};
    extent.width = std::clamp(static_cast<uint32_t>(width), caps.minImageExtent.width, caps.maxImageExtent.width);
    extent.height = std::clamp(static_cast<uint32_t>(height), caps.minImageExtent.height, caps.maxImageExtent.height);
    return extent;
}

VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect) {
    VkImageViewCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image = image;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.format = format;
    info.subresourceRange.aspectMask = aspect;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = 1;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;

    VkImageView view = VK_NULL_HANDLE;
    if (vkCreateImageView(device, &info, nullptr, &view) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateImageView failed");
    }
    return view;
}

uint32_t findMemoryType(VkPhysicalDevice phys, uint32_t type_filter, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(phys, &mem_props);

    for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i) {
        if ((type_filter & (1u << i)) &&
            (mem_props.memoryTypes[i].propertyFlags & props) == props) {
            return i;
        }
    }
    throw std::runtime_error("No suitable memory type found");
    return 0;
}

VkFence createFence(VkDevice device, bool signaled) {
    VkFenceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

    VkFence fence = VK_NULL_HANDLE;
    if (vkCreateFence(device, &info, nullptr, &fence) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateFence failed");
    }
    return fence;
}

VkCommandPool createCommandPool(VkDevice device, uint32_t queue_family, VkCommandPoolCreateFlags flags) {
    VkCommandPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.queueFamilyIndex = queue_family;
    info.flags = flags;

    VkCommandPool pool = VK_NULL_HANDLE;
    if (vkCreateCommandPool(device, &info, nullptr, &pool) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateCommandPool failed");
    }
    return pool;
}

VkFormat findDepthFormat(VkPhysicalDevice phys) {
    std::vector<VkFormat> candidates = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };

    for (auto fmt : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(phys, fmt, &props);
        if ((props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0) {
            return fmt;
        }
    }
    throw std::runtime_error("No suitable depth format found");
    return VK_FORMAT_UNDEFINED;
}

}
