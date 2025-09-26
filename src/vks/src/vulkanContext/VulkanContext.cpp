
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "VulkanContext.hpp"
#include <stdexcept>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_win32.h>

namespace vks {

VulkanContext::VulkanContext() {
    createInstance();
    setupDebugMessenger();
    createLogicalDevice();
}

VulkanContext::~VulkanContext() {


    if (debugMessenger != VK_NULL_HANDLE) {
        vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    vkDestroyInstance(instance, nullptr);
}

bool VulkanContext::initialize()
{
    // Create instance
    if (createInstance() != VK_SUCCESS) {
        return false;
    }

    // Setup debug messenger if validation is enabled
    if (enableValidation) {
        setupDebugMessenger();
    }

    // Create logical device and get queues
    createLogicalDevice();
    if (device == nullptr) {
        return false;
    }

    return true;
}

void VulkanContext::cleanup()
{

    if (debugMessenger != VK_NULL_HANDLE) {
        vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        debugMessenger = VK_NULL_HANDLE;
    }

    if (instance != VK_NULL_HANDLE) {
        vkDestroyInstance(instance, nullptr);
        instance = VK_NULL_HANDLE;
    }
}

// ... existing instance creation code ...

void VulkanContext::createLogicalDevice() {
    // First select physical device
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    // For now just take first device
    VkPhysicalDevice physicalDevice = devices[0];

    // Create the vulkan device wrapper
    device = base::VulkanDevice(physicalDevice);

    // Enable required features and extensions
    VkPhysicalDeviceFeatures enabledFeatures{};
    std::vector<const char*> enabledExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    // Create the logical device
    VkResult result = device.createLogicalDevice(enabledFeatures, enabledExtensions);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

}

// ... rest of the existing code ...

} // namespace vks