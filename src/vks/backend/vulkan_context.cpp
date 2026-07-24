#include "vks/backend/vulkan_context.hpp"

#include "vks/backend/sdl_window.hpp"
#include "vks/backend/vulkan_utils.hpp"

#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace vks::backend {

namespace {

std::vector<const char*> gather_instance_extensions() {
    Uint32 sdl_ext_count = 0;
    const char* const* sdl_exts = SDL_Vulkan_GetInstanceExtensions(&sdl_ext_count);
    if (!sdl_exts) {
        throw std::runtime_error("SDL_Vulkan_GetInstanceExtensions failed: " + std::string(SDL_GetError()));
    }

    std::vector<const char*> extensions;
    extensions.reserve(sdl_ext_count + 1);
    for (Uint32 i = 0; i < sdl_ext_count; ++i) {
        extensions.push_back(sdl_exts[i]);
    }

#if defined(_WIN32)
    extensions.push_back("VK_KHR_win32_surface");
#endif
#if defined(__linux__)
    extensions.push_back("VK_KHR_xlib_surface");
#endif

    return extensions;
}

std::vector<const char*> gather_instance_layers(bool enable_validation) {
    if (!enable_validation) return {};
    return { "VK_LAYER_KHRONOS_validation" };
}

VkInstance build_instance(const std::vector<const char*>& extensions,
                          const std::vector<const char*>& layers) {
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "VulkanSimplified";
    app_info.applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
    app_info.pEngineName = "VulkanSimplified Engine";
    app_info.engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info.pApplicationInfo = &app_info;
    info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    info.ppEnabledExtensionNames = extensions.data();
    info.enabledLayerCount = static_cast<uint32_t>(layers.size());
    info.ppEnabledLayerNames = layers.data();

    VkInstance instance = VK_NULL_HANDLE;
    if (vkCreateInstance(&info, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateInstance failed");
    }
    return instance;
}

VkSurfaceKHR build_surface(VkInstance instance, SDL_Window* window) {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface)) {
        throw std::runtime_error("SDL_Vulkan_CreateSurface: " + std::string(SDL_GetError()));
    }
    return surface;
}

VkPhysicalDevice pick_phys_device(VkInstance instance) {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    if (count == 0) throw std::runtime_error("No Vulkan GPUs found");

    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(instance, &count, devices.data());

    VkPhysicalDevice chosen = VK_NULL_HANDLE;
    int best_score = 0;

    for (const auto& dev : devices) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(dev, &props);

        if (VK_API_VERSION_MAJOR(props.apiVersion) < 1 ||
            VK_API_VERSION_MINOR(props.apiVersion) < 3) {
            continue;
        }

        VkPhysicalDeviceFeatures2 f2{};
        f2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        VkPhysicalDeviceSynchronization2Features s2{};
        s2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
        s2.pNext = f2.pNext;
        f2.pNext = &s2;
        VkPhysicalDeviceDynamicRenderingFeatures dr{};
        dr.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
        dr.pNext = s2.pNext;
        s2.pNext = &dr;
        vkGetPhysicalDeviceFeatures2(dev, &f2);

        if (!s2.synchronization2 || !dr.dynamicRendering) continue;

        int score = 0;
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) score += 10000;
        else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) score += 1000;
        score += static_cast<int>(props.limits.maxImageDimension2D);

        if (score > best_score) {
            best_score = score;
            chosen = dev;
        }
    }

    if (chosen == VK_NULL_HANDLE) {
        throw std::runtime_error("No suitable Vulkan 1.3 GPU found");
    }
    return chosen;
}

VkDevice build_device(VkPhysicalDevice phys,
                      const QueueFamilyIndices& families,
                      bool headless) {
    std::vector<VkDeviceQueueCreateInfo> q_infos;
    float priority = 1.0f;

    std::vector<uint32_t> uniq;
    uniq.push_back(families.graphics.value());
    if (families.present != families.graphics) uniq.push_back(families.present.value());
    if (families.transfer.has_value() &&
        families.transfer.value() != families.graphics.value() &&
        families.transfer.value() != families.present.value()) {
        uniq.push_back(families.transfer.value());
    }

    for (uint32_t idx : uniq) {
        VkDeviceQueueCreateInfo qi{};
        qi.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qi.queueFamilyIndex = idx;
        qi.queueCount = 1;
        qi.pQueuePriorities = &priority;
        q_infos.push_back(qi);
    }

    VkPhysicalDeviceSynchronization2Features sync2{};
    sync2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    sync2.synchronization2 = VK_TRUE;

    VkPhysicalDeviceDynamicRenderingFeatures dr{};
    dr.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dr.dynamicRendering = VK_TRUE;
    dr.pNext = &sync2;

    VkPhysicalDeviceFeatures2 f2{};
    f2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    f2.pNext = &dr;

    std::vector<const char*> dev_exts = {
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
    };
    if (!headless) {
        dev_exts.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }

    VkDeviceCreateInfo dci{};
    dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.pNext = &f2;
    dci.queueCreateInfoCount = static_cast<uint32_t>(q_infos.size());
    dci.pQueueCreateInfos = q_infos.data();
    dci.enabledExtensionCount = static_cast<uint32_t>(dev_exts.size());
    dci.ppEnabledExtensionNames = dev_exts.data();
    dci.pEnabledFeatures = nullptr;

    VkDevice dev = VK_NULL_HANDLE;
    if (vkCreateDevice(phys, &dci, nullptr, &dev) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateDevice failed");
    }
    return dev;
}

VkSwapchainKHR build_swapchain(VkDevice dev,
                               VkPhysicalDevice phys,
                               VkSurfaceKHR surface,
                               VkFormat fmt,
                               VkExtent2D extent,
                               uint32_t graphics_fam,
                               uint32_t present_fam) {
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys, surface, &caps);

    uint32_t img_count = std::clamp(caps.minImageCount + 1u, 1u, caps.maxImageCount);

    VkSwapchainCreateInfoKHR sci{};
    sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sci.surface = surface;
    sci.minImageCount = img_count;
    sci.imageFormat = fmt;
    sci.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    sci.imageExtent = extent;
    sci.imageArrayLayers = 1;
    sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    uint32_t q_fams[2]{};
    if (graphics_fam != present_fam) {
        sci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        q_fams[0] = graphics_fam;
        q_fams[1] = present_fam;
        sci.queueFamilyIndexCount = 2;
        sci.pQueueFamilyIndices = q_fams;
    }

    sci.preTransform = caps.currentTransform;
    sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    sci.clipped = VK_TRUE;
    sci.oldSwapchain = VK_NULL_HANDLE;

    VkSwapchainKHR swap = VK_NULL_HANDLE;
    if (vkCreateSwapchainKHR(dev, &sci, nullptr, &swap) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateSwapchainKHR failed");
    }
    return swap;
}

VkRenderPass build_render_pass(VkDevice dev, VkFormat colorFmt) {
    VkAttachmentDescription color{};
    color.format = colorFmt;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depth{};
    depth.format = VK_FORMAT_D32_SFLOAT;
    depth.samples = VK_SAMPLE_COUNT_1_BIT;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthRef{};
    depthRef.attachment = 1;
    depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    subpass.pDepthStencilAttachment = &depthRef;

    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription attachments[] = {color, depth};
    VkRenderPassCreateInfo rpCI{};
    rpCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpCI.attachmentCount = 2;
    rpCI.pAttachments = attachments;
    rpCI.subpassCount = 1;
    rpCI.pSubpasses = &subpass;
    rpCI.dependencyCount = 1;
    rpCI.pDependencies = &dep;

    VkRenderPass rp = VK_NULL_HANDLE;
    if (vkCreateRenderPass(dev, &rpCI, nullptr, &rp) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateRenderPass failed");
    }
    return rp;
}

}

struct VulkanContext::Impl {
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphics_queue = VK_NULL_HANDLE;
    VkQueue present_queue = VK_NULL_HANDLE;
    VkQueue transfer_queue = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkCommandPool graphics_cmd_pool = VK_NULL_HANDLE;
    VkCommandPool transfer_cmd_pool = VK_NULL_HANDLE;
    VkFormat swapchain_format = VK_FORMAT_UNDEFINED;
    VkExtent2D swapchain_extent{};
    uint32_t graphics_family = 0;
    uint32_t present_family = 0;
    uint32_t transfer_family = 0;
    bool headless = false;
    bool enable_validation = false;
    bool valid = false;

    std::vector<SwapchainImage> swapchain_images;
    SDL_Window* sdl_window = nullptr;

    VkPipelineCache pipeline_cache = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkRenderPass render_pass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> swapchainFramebuffers;

    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    VkImageView depthImageView = VK_NULL_HANDLE;

    uint32_t current_frame = 0;
    uint32_t current_image_index = 0;
    VkCommandBuffer current_command_buffer = VK_NULL_HANDLE;
    VkSemaphore acquire_semaphore = VK_NULL_HANDLE;
};

VulkanContext::VulkanContext(SdlWindow& window, bool headless, bool enable_validation)
    : impl_(std::make_unique<Impl>())
{
    impl_->sdl_window = window.nativeHandle();
    impl_->headless = headless;
    impl_->enable_validation = enable_validation;

    auto extensions = gather_instance_extensions();
    auto layers     = gather_instance_layers(enable_validation);

    impl_->instance    = build_instance(extensions, layers);
    impl_->surface     = build_surface(impl_->instance, impl_->sdl_window);
    impl_->physical_device = pick_phys_device(impl_->instance);

    QueueFamilyIndices families = findQueueFamilies(impl_->physical_device, impl_->surface);
    impl_->graphics_family  = families.graphics.value();
    impl_->present_family   = families.present.value();
    impl_->transfer_family  = families.transfer.value_or(impl_->graphics_family);

    impl_->device = build_device(impl_->physical_device, families, headless);

    vkGetDeviceQueue(impl_->device, impl_->graphics_family, 0, &impl_->graphics_queue);
    vkGetDeviceQueue(impl_->device, impl_->present_family,  0, &impl_->present_queue);
    if (families.transfer.has_value() &&
        families.transfer.value() != impl_->graphics_family &&
        families.transfer.value() != impl_->present_family) {
        vkGetDeviceQueue(impl_->device, impl_->transfer_family, 0, &impl_->transfer_queue);
    } else {
        impl_->transfer_queue = impl_->graphics_queue;
    }

    impl_->graphics_cmd_pool = createCommandPool(
        impl_->device, impl_->graphics_family,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
    );
    if (families.transfer.has_value() &&
        families.transfer.value() != impl_->graphics_family) {
        impl_->transfer_cmd_pool = createCommandPool(impl_->device, impl_->transfer_family);
    } else {
        impl_->transfer_cmd_pool = impl_->graphics_cmd_pool;
    }

    VkSemaphoreCreateInfo semCI{};
    semCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkCreateSemaphore(impl_->device, &semCI, nullptr, &impl_->acquire_semaphore);

    if (!headless) {
        int w = -1, h = -1;
        SDL_GetWindowSize(impl_->sdl_window, &w, &h);

        impl_->swapchain_format = pickSurfaceFormat(impl_->physical_device, impl_->surface).format;

        VkSurfaceCapabilitiesKHR caps;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(impl_->physical_device, impl_->surface, &caps);
        impl_->swapchain_extent = pickSwapExtent(caps, w, h);

        impl_->swapchain = build_swapchain(
            impl_->device, impl_->physical_device, impl_->surface,
            impl_->swapchain_format, impl_->swapchain_extent,
            impl_->graphics_family, impl_->present_family
        );

        uint32_t n = 0;
        vkGetSwapchainImagesKHR(impl_->device, impl_->swapchain, &n, nullptr);
        std::vector<VkImage> raw(n);
        vkGetSwapchainImagesKHR(impl_->device, impl_->swapchain, &n, raw.data());

        for (uint32_t i = 0; i < n; ++i) {
            SwapchainImage si;
            si.image      = raw[i];
            si.index      = i;
            si.image_view = createImageView(si.image,
                                            impl_->swapchain_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
            impl_->swapchain_images.push_back(si);
        }

        impl_->render_pass = build_render_pass(impl_->device, impl_->swapchain_format);

        createDepthResources();
        createFramebuffers();

        impl_->valid = true;
    } else {
        impl_->swapchain_format = VK_FORMAT_R8G8B8A8_UNORM;
        SDL_GetWindowSize(impl_->sdl_window,
                          reinterpret_cast<int*>(&impl_->swapchain_extent.width),
                          reinterpret_cast<int*>(&impl_->swapchain_extent.height));
        impl_->valid = true;
    }
}

VulkanContext::~VulkanContext() {
    if (!impl_) return;

    if (impl_->device) {
        vkDeviceWaitIdle(impl_->device);
        for (auto& si : impl_->swapchain_images) {
            if (si.image_view) vkDestroyImageView(impl_->device, si.image_view, nullptr);
        }
        if (impl_->swapchain) vkDestroySwapchainKHR(impl_->device, impl_->swapchain, nullptr);
        if (impl_->transfer_cmd_pool && impl_->transfer_cmd_pool != impl_->graphics_cmd_pool) {
            vkDestroyCommandPool(impl_->device, impl_->transfer_cmd_pool, nullptr);
        }
        if (impl_->graphics_cmd_pool) vkDestroyCommandPool(impl_->device, impl_->graphics_cmd_pool, nullptr);
        if (impl_->descriptorPool) vkDestroyDescriptorPool(impl_->device, impl_->descriptorPool, nullptr);
        if (impl_->descriptorSetLayout) vkDestroyDescriptorSetLayout(impl_->device, impl_->descriptorSetLayout, nullptr);
        if (impl_->pipeline_cache) vkDestroyPipelineCache(impl_->device, impl_->pipeline_cache, nullptr);
        if (impl_->render_pass) vkDestroyRenderPass(impl_->device, impl_->render_pass, nullptr);
        if (impl_->acquire_semaphore) vkDestroySemaphore(impl_->device, impl_->acquire_semaphore, nullptr);
        if (impl_->depthImageView) vkDestroyImageView(impl_->device, impl_->depthImageView, nullptr);
        if (impl_->depthImage) vkDestroyImage(impl_->device, impl_->depthImage, nullptr);
        if (impl_->depthImageMemory) vkFreeMemory(impl_->device, impl_->depthImageMemory, nullptr);
        for (auto fb : impl_->swapchainFramebuffers) {
            if (fb) vkDestroyFramebuffer(impl_->device, fb, nullptr);
        }
        vkDestroyDevice(impl_->device, nullptr);
    }
    if (impl_->surface) vkDestroySurfaceKHR(impl_->instance, impl_->surface, nullptr);
    if (impl_->instance) vkDestroyInstance(impl_->instance, nullptr);
    impl_->valid = false;
}

VulkanContext::VulkanContext(VulkanContext&& other) noexcept : impl_(std::move(other.impl_)) {
    other.impl_ = nullptr;
}

VulkanContext& VulkanContext::operator=(VulkanContext&& other) noexcept {
    if (this != &other) {
        impl_ = std::move(other.impl_);
        other.impl_ = nullptr;
    }
    return *this;
}

bool VulkanContext::recreateSwapchain() {
    if (!impl_ || impl_->headless) return true;

    if (impl_->swapchain) {
        for (auto& si : impl_->swapchain_images) {
            if (si.image_view) vkDestroyImageView(impl_->device, si.image_view, nullptr);
        }
        vkDestroySwapchainKHR(impl_->device, impl_->swapchain, nullptr);
        impl_->swapchain = VK_NULL_HANDLE;
    }
    for (auto fb : impl_->swapchainFramebuffers) {
        if (fb) vkDestroyFramebuffer(impl_->device, fb, nullptr);
    }
    impl_->swapchainFramebuffers.clear();
    if (impl_->depthImageView) vkDestroyImageView(impl_->device, impl_->depthImageView, nullptr);
    if (impl_->depthImage) vkDestroyImage(impl_->device, impl_->depthImage, nullptr);
    if (impl_->depthImageMemory) vkFreeMemory(impl_->device, impl_->depthImageMemory, nullptr);
    impl_->depthImageView = VK_NULL_HANDLE;
    impl_->depthImage = VK_NULL_HANDLE;
    impl_->depthImageMemory = VK_NULL_HANDLE;
    impl_->swapchain_images.clear();

    int w = -1, h = -1;
    SDL_GetWindowSize(impl_->sdl_window, &w, &h);
    if (w <= 0 || h <= 0) return false;

    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(impl_->physical_device, impl_->surface, &caps);
    impl_->swapchain_extent = pickSwapExtent(caps, w, h);
    impl_->swapchain_format = pickSurfaceFormat(impl_->physical_device, impl_->surface).format;

    uint32_t img_count = std::clamp(caps.minImageCount + 1u, 1u, caps.maxImageCount);
    impl_->swapchain = build_swapchain(
        impl_->device, impl_->physical_device, impl_->surface,
        impl_->swapchain_format, impl_->swapchain_extent,
        impl_->graphics_family, impl_->present_family
    );

    uint32_t actual = 0;
    vkGetSwapchainImagesKHR(impl_->device, impl_->swapchain, &actual, nullptr);
    std::vector<VkImage> raw(actual);
    vkGetSwapchainImagesKHR(impl_->device, impl_->swapchain, &actual, raw.data());

    for (uint32_t i = 0; i < actual; ++i) {
        SwapchainImage si;
        si.image      = raw[i];
        si.index      = i;
        si.image_view = createImageView(si.image,
                                        impl_->swapchain_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        impl_->swapchain_images.push_back(si);
    }

    createDepthResources();
    createFramebuffers();

    return true;
}

VkResult VulkanContext::acquireNextImage(uint64_t timeout_ns, uint32_t* out_index) {
    if (!impl_ || !out_index) return VK_ERROR_INITIALIZATION_FAILED;
    if (impl_->headless || impl_->swapchain == VK_NULL_HANDLE) {
        *out_index = 0;
        return VK_SUCCESS;
    }
    return vkAcquireNextImageKHR(impl_->device, impl_->swapchain, timeout_ns,
                                  VK_NULL_HANDLE, VK_NULL_HANDLE, out_index);
}

bool VulkanContext::present(uint32_t image_index) {
    if (!impl_ || impl_->headless) return true;

    VkPresentInfoKHR pi{};
    pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.swapchainCount = 1;
    pi.pSwapchains    = &impl_->swapchain;
    pi.pImageIndices  = &image_index;

    VkResult r = vkQueuePresentKHR(impl_->present_queue, &pi);
    if (r == VK_ERROR_OUT_OF_DATE_KHR || r == VK_SUBOPTIMAL_KHR) {
        return recreateSwapchain();
    }
    return r == VK_SUCCESS;
}

bool VulkanContext::present() {
    if (!impl_ || impl_->headless) return true;
    return true;
}

bool VulkanContext::isValid()                   const { return impl_ && impl_->valid; }
VkFormat VulkanContext::swapchainImageFormat()  const { return impl_ ? impl_->swapchain_format : VK_FORMAT_UNDEFINED; }
VkExtent2D VulkanContext::swapchainExtent()     const { return impl_ ? impl_->swapchain_extent : VkExtent2D{}; }
bool VulkanContext::isHeadless()                const { return impl_ ? impl_->headless : false; }

VkInstance        VulkanContext::instance()          const { return impl_ ? impl_->instance         : VK_NULL_HANDLE; }
VkDevice          VulkanContext::device()            const { return impl_ ? impl_->device            : VK_NULL_HANDLE; }
VkPhysicalDevice  VulkanContext::physicalDevice()    const { return impl_ ? impl_->physical_device  : VK_NULL_HANDLE; }
VkQueue           VulkanContext::graphicsQueue()     const { return impl_ ? impl_->graphics_queue    : VK_NULL_HANDLE; }
VkQueue           VulkanContext::presentQueue()      const { return impl_ ? impl_->present_queue     : VK_NULL_HANDLE; }
uint32_t          VulkanContext::graphicsFamily()    const { return impl_ ? impl_->graphics_family   : 0; }
uint32_t          VulkanContext::presentFamily()     const { return impl_ ? impl_->present_family    : 0; }
VkCommandPool     VulkanContext::graphicsCommandPool() const { return impl_ ? impl_->graphics_cmd_pool : VK_NULL_HANDLE; }
VkCommandPool     VulkanContext::transferCommandPool() const { return impl_ ? impl_->transfer_cmd_pool : VK_NULL_HANDLE; }

VkDescriptorSetLayout VulkanContext::createDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo& createInfo) {
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    vkCreateDescriptorSetLayout(device(), &createInfo, nullptr, &layout);
    impl_->descriptorSetLayout = layout;
    return layout;
}

VkDescriptorSetLayout VulkanContext::createDescriptorSetLayout(const VkDescriptorSetLayoutBinding* bindings, uint32_t bindingCount) {
    VkDescriptorSetLayoutCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ci.bindingCount = bindingCount;
    ci.pBindings = bindings;
    return createDescriptorSetLayout(ci);
}

VkPipelineLayout VulkanContext::createPipelineLayout(const VkDescriptorSetLayout* setLayouts, uint32_t setCount,
                                                      const VkPushConstantRange* pushRanges, uint32_t pushCount) {
    VkPipelineLayoutCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    ci.setLayoutCount = setCount;
    ci.pSetLayouts = setLayouts;
    ci.pushConstantRangeCount = pushCount;
    ci.pPushConstantRanges = pushRanges;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    vkCreatePipelineLayout(device(), &ci, nullptr, &layout);
    return layout;
}

VkPipelineLayout VulkanContext::createPipelineLayout(const VkDescriptorSetLayout& setLayout) {
    return createPipelineLayout(&setLayout, 1, nullptr, 0);
}

VkShaderModule VulkanContext::createShaderModule(std::span<const uint32_t> spirv) {
    if (spirv.empty()) return VK_NULL_HANDLE;
    VkShaderModuleCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ci.codeSize = spirv.size_bytes();
    ci.pCode = spirv.data();
    VkShaderModule module = VK_NULL_HANDLE;
    vkCreateShaderModule(device(), &ci, nullptr, &module);
    return module;
}

VkPipeline VulkanContext::createGraphicsPipeline(const VkPipelineShaderStageCreateInfo* stages, uint32_t stageCount,
                                                  const VkPipelineVertexInputStateCreateInfo* vertexInput,
                                                  VkPipelineLayout layout, VkRenderPass renderPass,
                                                  VkPrimitiveTopology topology, bool depthTest, bool depthWrite,
                                                  VkCullModeFlags cullMode, bool blendEnable) {
    VkPipelineInputAssemblyStateCreateInfo ia{};
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = topology;

    VkViewport vp{};
    vp.width = static_cast<float>(swapchainExtent().width);
    vp.height = static_cast<float>(swapchainExtent().height);
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.extent = swapchainExtent();

    VkPipelineViewportStateCreateInfo vpState{};
    vpState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vpState.viewportCount = 1;
    vpState.pViewports = &vp;
    vpState.scissorCount = 1;
    vpState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rs{};
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.cullMode = cullMode;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo ds{};
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = depthTest ? VK_TRUE : VK_FALSE;
    ds.depthWriteEnable = depthWrite ? VK_TRUE : VK_FALSE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState cbAtt{};
    cbAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                           VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    cbAtt.blendEnable = blendEnable ? VK_TRUE : VK_FALSE;

    VkPipelineColorBlendStateCreateInfo cb{};
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb.attachmentCount = 1;
    cb.pAttachments = &cbAtt;

    VkGraphicsPipelineCreateInfo gpi{};
    gpi.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gpi.stageCount = stageCount;
    gpi.pStages = stages;
    gpi.pVertexInputState = vertexInput;
    gpi.pInputAssemblyState = &ia;
    gpi.pViewportState = &vpState;
    gpi.pRasterizationState = &rs;
    gpi.pMultisampleState = &ms;
    gpi.pDepthStencilState = &ds;
    gpi.pColorBlendState = &cb;
    gpi.layout = layout;
    gpi.renderPass = renderPass;
    gpi.subpass = 0;

    VkPipeline pipeline = VK_NULL_HANDLE;
    vkCreateGraphicsPipelines(device(), VK_NULL_HANDLE, 1, &gpi, nullptr, &pipeline);
    return pipeline;
}

VkRenderPass VulkanContext::renderPass() const {
    return impl_ ? impl_->render_pass : VK_NULL_HANDLE;
}

VkImage VulkanContext::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format,
                                    VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags props, VkDeviceMemory& outMemory) {
    VkImageCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ci.imageType = VK_IMAGE_TYPE_2D;
    ci.format = format;
    ci.extent = {width, height, 1};
    ci.mipLevels = mipLevels;
    ci.arrayLayers = 1;
    ci.samples = VK_SAMPLE_COUNT_1_BIT;
    ci.tiling = tiling;
    ci.usage = usage;
    ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImage img = VK_NULL_HANDLE;
    if (vkCreateImage(device(), &ci, nullptr, &img) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateImage failed");
    }

    VkMemoryRequirements reqs;
    vkGetImageMemoryRequirements(device(), img, &reqs);

    VkMemoryAllocateInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize = reqs.size;
    ai.memoryTypeIndex = findMemoryType(physicalDevice(), reqs.memoryTypeBits, props);

    if (vkAllocateMemory(device(), &ai, nullptr, &outMemory) != VK_SUCCESS) {
        vkDestroyImage(device(), img, nullptr);
        throw std::runtime_error("vkAllocateMemory failed for image");
    }

    vkBindImageMemory(device(), img, outMemory, 0);
    return img;
}

VkImageView VulkanContext::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect, uint32_t mipLevels) {
    VkImageViewCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ci.image = image;
    ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ci.format = format;
    ci.subresourceRange.aspectMask = aspect;
    ci.subresourceRange.baseMipLevel = 0;
    ci.subresourceRange.levelCount = mipLevels;
    ci.subresourceRange.baseArrayLayer = 0;
    ci.subresourceRange.layerCount = 1;

    VkImageView view = VK_NULL_HANDLE;
    if (vkCreateImageView(device(), &ci, nullptr, &view) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateImageView failed");
    }
    return view;
}

VkSampler VulkanContext::createSampler() {
    VkSamplerCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    ci.magFilter = VK_FILTER_LINEAR;
    ci.minFilter = VK_FILTER_LINEAR;
    ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    ci.maxAnisotropy = 16.0f;
    ci.maxLod = 16.0f;

    VkSampler sampler = VK_NULL_HANDLE;
    if (vkCreateSampler(device(), &ci, nullptr, &sampler) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateSampler failed");
    }
    return sampler;
}

VkBuffer VulkanContext::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                      VkMemoryPropertyFlags props, VkDeviceMemory& outMemory) {
    VkBufferCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    ci.size = size;
    ci.usage = usage;
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer buf = VK_NULL_HANDLE;
    if (vkCreateBuffer(device(), &ci, nullptr, &buf) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateBuffer failed");
    }

    VkMemoryRequirements reqs;
    vkGetBufferMemoryRequirements(device(), buf, &reqs);

    VkMemoryAllocateInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize = reqs.size;
    ai.memoryTypeIndex = findMemoryType(physicalDevice(), reqs.memoryTypeBits, props);

    if (vkAllocateMemory(device(), &ai, nullptr, &outMemory) != VK_SUCCESS) {
        vkDestroyBuffer(device(), buf, nullptr);
        throw std::runtime_error("vkAllocateMemory failed for buffer");
    }

    vkBindBufferMemory(device(), buf, outMemory, 0);
    return buf;
}

void* VulkanContext::mapBuffer(VkBuffer /*unused*/, VkDeviceMemory memory, VkDeviceSize /*unused*/) {
    void* ptr = nullptr;
    vkMapMemory(device(), memory, 0, VK_WHOLE_SIZE, 0, &ptr);
    return ptr;
}

void VulkanContext::unmapBuffer(VkDeviceMemory memory) {
    vkUnmapMemory(device(), memory);
}

VkFormat VulkanContext::findDepthFormat(VkPhysicalDevice phys) {
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

void VulkanContext::createDepthResources() {
    if (!impl_ || impl_->headless) return;

    auto* dev = device();
    VkExtent2D extent = swapchainExtent();
    VkFormat depthFormat = findDepthFormat(physicalDevice());

    VkImageCreateInfo depthCI{};
    depthCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depthCI.imageType = VK_IMAGE_TYPE_2D;
    depthCI.format = depthFormat;
    depthCI.extent = {extent.width, extent.height, 1};
    depthCI.mipLevels = 1;
    depthCI.arrayLayers = 1;
    depthCI.samples = VK_SAMPLE_COUNT_1_BIT;
    depthCI.tiling = VK_IMAGE_TILING_OPTIMAL;
    depthCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depthCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(dev, &depthCI, nullptr, &impl_->depthImage) != VK_SUCCESS) return;

    VkMemoryRequirements depthReqs;
    vkGetImageMemoryRequirements(dev, impl_->depthImage, &depthReqs);

    VkMemoryAllocateInfo depthAI{};
    depthAI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    depthAI.allocationSize = depthReqs.size;
    depthAI.memoryTypeIndex = findMemoryType(physicalDevice(), depthReqs.memoryTypeBits,
                                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(dev, &depthAI, nullptr, &impl_->depthImageMemory) != VK_SUCCESS) {
        vkDestroyImage(dev, impl_->depthImage, nullptr);
        impl_->depthImage = VK_NULL_HANDLE;
        return;
    }
    vkBindImageMemory(dev, impl_->depthImage, impl_->depthImageMemory, 0);

    impl_->depthImageView = createImageView(impl_->depthImage, depthFormat,
                                            VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}

void VulkanContext::createFramebuffers() {
    if (!impl_ || impl_->headless) return;

    auto* dev = device();
    VkExtent2D extent = swapchainExtent();

    for (auto& si : impl_->swapchain_images) {
        VkImageView attachments[] = {si.image_view, impl_->depthImageView};

        VkFramebufferCreateInfo fbCI{};
        fbCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbCI.renderPass = impl_->render_pass;
        fbCI.attachmentCount = 2;
        fbCI.pAttachments = attachments;
        fbCI.width = extent.width;
        fbCI.height = extent.height;
        fbCI.layers = 1;

        VkFramebuffer fb = VK_NULL_HANDLE;
        vkCreateFramebuffer(dev, &fbCI, nullptr, &fb);
        impl_->swapchainFramebuffers.push_back(fb);
    }
}

VkImageView VulkanContext::depthImageView() const {
    return impl_ ? impl_->depthImageView : VK_NULL_HANDLE;
}

VkImage VulkanContext::depthImage() const {
    return impl_ ? impl_->depthImage : VK_NULL_HANDLE;
}

uint32_t VulkanContext::currentFrame() const {
    return impl_ ? impl_->current_frame : 0;
}

VkCommandBuffer VulkanContext::beginFrame() {
    if (!impl_) return VK_NULL_HANDLE;

    if (!impl_->headless && impl_->swapchain) {
        uint32_t idx = 0;
        VkResult r = vkAcquireNextImageKHR(impl_->device, impl_->swapchain, UINT64_MAX,
                                            impl_->acquire_semaphore, VK_NULL_HANDLE, &idx);
        if (r == VK_SUCCESS || r == VK_SUBOPTIMAL_KHR) {
            impl_->current_image_index = idx;
        }
    }

    VkCommandBufferAllocateInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandPool = impl_->graphics_cmd_pool;
    ai.commandBufferCount = 1;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    VkCommandBuffer cb = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(impl_->device, &ai, &cb);

    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cb, &bi);

    impl_->current_command_buffer = cb;
    return cb;
}

VkCommandBuffer VulkanContext::currentCommandBuffer() const {
    return impl_ ? impl_->current_command_buffer : VK_NULL_HANDLE;
}

void VulkanContext::endFrame() {
    if (!impl_ || impl_->current_command_buffer == VK_NULL_HANDLE) return;

    vkEndCommandBuffer(impl_->current_command_buffer);

    VkSubmitInfo si{};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &impl_->current_command_buffer;

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSemaphore waitSemaphores[] = {impl_->acquire_semaphore};
    si.waitSemaphoreCount = 1;
    si.pWaitSemaphores = waitSemaphores;
    si.pWaitDstStageMask = &waitStage;

    vkQueueSubmit(impl_->graphics_queue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(impl_->graphics_queue);

    uint32_t imageIndex = impl_->current_image_index;
    if (imageIndex < impl_->swapchain_images.size()) {
        present(imageIndex);
    }

    impl_->current_command_buffer = VK_NULL_HANDLE;
    impl_->current_frame++;
}

VkFramebuffer VulkanContext::currentFramebuffer() const {
    if (!impl_ || impl_->swapchainFramebuffers.empty()) return VK_NULL_HANDLE;
    uint32_t idx = impl_->current_image_index;
    if (idx >= impl_->swapchainFramebuffers.size()) return VK_NULL_HANDLE;
    return impl_->swapchainFramebuffers[idx];
}

void VulkanContext::updateDescriptorSets(std::span<const VkWriteDescriptorSet> writes) {
    if (!impl_) return;
    vkUpdateDescriptorSets(impl_->device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

VkPipelineCache VulkanContext::pipelineCache() const {
    return impl_ ? impl_->pipeline_cache : VK_NULL_HANDLE;
}

}