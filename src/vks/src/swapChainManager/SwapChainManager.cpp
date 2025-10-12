
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "SwapChainManager.hpp"
#include <algorithm>
#include <libloaderapi.h>
#include <limits>
#include <stdexcept>
#include <SDL3/SDL_video.h>
#include <vulkan/vulkan_win32.h>

#if defined(PLATFORM_SDL3)
#include <SDL3/SDL_vulkan.h>
#endif

namespace vks {

SwapChainManager::SwapChainManager(VulkanContext* context) : context(context) {
}

SwapChainManager::~SwapChainManager() {
    cleanup();
}

void SwapChainManager::cleanup() {
    if (context->getDevice() != VK_NULL_HANDLE) {
        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(context->getDevice(), imageView, nullptr);
        }
        vkDestroySwapchainKHR(context->getDevice(), swapChain, nullptr);
    }
    if (surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(context->getInstance(), surface, nullptr);
    }
}

void SwapChainManager::createSurface(void* windowHandle) {
#if defined(PLATFORM_SDL3)

    if (!SDL_Vulkan_CreateSurface(reinterpret_cast<SDL_Window*>(windowHandle),context->getInstance(),
                                      nullptr,
                                      &surface))
    {
        throw std::runtime_error("failed to create window surface!");
    }
#endif
}


void SwapChainManager::recreateSwapChain(uint32_t width, uint32_t height) {
    windowHeight = height;
    windowWidth = width;

    auto device = context->getDevice();
    vkDeviceWaitIdle(device);

    // Cleanup old swap chain resources
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    // Create new swap chain
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->getPhysicalDevice(), surface, &capabilities);

    VkExtent2D extent = {};
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        extent = capabilities.currentExtent;
    } else {
        extent.width = std::clamp(width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width);
        extent.height = std::clamp(height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height);
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = capabilities.minImageCount + 1;
    createInfo.imageFormat = swapChainImageFormat;
    createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = swapChain;

    VkSwapchainKHR newSwapChain;
    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &newSwapChain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swap chain!");
    }

    swapChain = newSwapChain;
    swapChainExtent = extent;

    // Get the new swap chain images
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    // Create new image views
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = swapChainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = swapChainImageFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &viewInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image views!");
        }
    }
}

    void SwapChainManager::createSwapChain(uint32_t width, uint32_t height) {
    windowHeight = height;
    windowWidth = width;

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->getPhysicalDevice(), surface, &capabilities);

    // Get supported surface formats
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(context->getPhysicalDevice(), surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> availableFormats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(context->getPhysicalDevice(), surface, &formatCount, availableFormats.data());

    // Get supported present modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(context->getPhysicalDevice(), surface, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> availablePresentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(context->getPhysicalDevice(), surface, &presentModeCount, availablePresentModes.data());

    VkExtent2D extent = chooseSwapExtent(capabilities, width, height);
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(availableFormats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(availablePresentModes);

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = swapChainImageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = swapChainExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    auto resoult = vkCreateSwapchainKHR(context->getDevice(), &createInfo, nullptr, &swapChain);
    if (resoult != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    // Get swap chain images
    vkGetSwapchainImagesKHR(context->getDevice(), swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(context->getDevice(), swapChain, &imageCount, swapChainImages.data());

    createImageViews();
}


VkSurfaceFormatKHR SwapChainManager::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    // First, look for SRGB format as it's the most accurate color space
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
            }
    }

    // If we can't find our preferred format, just return the first available one
    return availableFormats[0];
}

VkPresentModeKHR SwapChainManager::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    // Look for mailbox mode (triple buffering) as it's the most optimal for gaming applications
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    // FIFO mode is guaranteed to be available
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapChainManager::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = {width, height};

        actualExtent.width = std::clamp(actualExtent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height);

        return actualExtent;
    }
}


void SwapChainManager::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapChainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(context->getDevice(), &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views!");
        }
    }
}

    VkResult SwapChainManager::acquireNextImage(VkSemaphore imageAvailableSemaphore) {

    uint32_t tmpImageIndex;
    VkResult result = vkAcquireNextImageKHR(context->getDevice(), swapChain, UINT64_MAX,
                                           imageAvailableSemaphore, VK_NULL_HANDLE, &tmpImageIndex);
    if (result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR) {
        currentImageIndex = tmpImageIndex;
    } else {
        // If acquire failed, mark no image as current
        currentImageIndex = UINT32_MAX;
    }

    return result;
}

    VkResult SwapChainManager::queuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore) {
    // Validate that the image was actually acquired
    if (imageIndex != currentImageIndex || currentImageIndex == UINT32_MAX) {
        return VK_ERROR_OUT_OF_DATE_KHR;
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = waitSemaphore ? 1 : 0;  // Only wait if semaphore provided
    presentInfo.pWaitSemaphores = waitSemaphore ? &waitSemaphore : nullptr;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    VkResult result = vkQueuePresentKHR(queue, &presentInfo);

    return result;
}
} // namespace vks