
#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include "../base/VulkanDevice.h"

namespace vks {
    class VulkanContext {
    public:
        VulkanContext();
        ~VulkanContext();

        void cleanup();

        // Getters
        VkInstance getInstance() const { return instance; }
        VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }
        base::VulkanDevice& getDevice() const { return *device; }

        VkQueue getGraphicsQueue() const { return graphicsQueue; }
        VkQueue getTransferQueue() const { return transferQueue; }

        const VkPhysicalDeviceProperties& getDeviceProperties() const { return deviceProperties; }
        const VkPhysicalDeviceFeatures& getDeviceFeatures() const { return deviceFeatures; }
        const VkPhysicalDeviceMemoryProperties& getMemoryProperties() const { return memoryProperties; }

    private:
        // Instance related
        VkInstance instance{VK_NULL_HANDLE};
        VkDebugUtilsMessengerEXT debugMessenger{VK_NULL_HANDLE};
        std::vector<const char*> enabledLayers;
        std::vector<const char*> enabledExtensions;

        // Device related
        VkPhysicalDevice physicalDevice{VK_NULL_HANDLE};
        base::VulkanDevice *device{};
        VkQueue graphicsQueue{VK_NULL_HANDLE};
        VkQueue transferQueue{VK_NULL_HANDLE};

        // Device properties
        VkPhysicalDeviceProperties deviceProperties{};
        VkPhysicalDeviceFeatures deviceFeatures{};
        VkPhysicalDeviceMemoryProperties memoryProperties{};


        VkResult createInstance();
        void setupDebugMessenger();
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createQueues();

        bool isDeviceSuitable(VkPhysicalDevice device);
        int rateDeviceSuitability(VkPhysicalDevice device);

        // Debugging

        bool enableValidation = true;

       static const char* vkResultToString(VkResult result);

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData);

        static VkResult CreateDebugUtilsMessengerEXT(
            VkInstance instance,
            const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkDebugUtilsMessengerEXT* pDebugMessenger);

        static void DestroyDebugUtilsMessengerEXT(
            VkInstance instance,
            VkDebugUtilsMessengerEXT debugMessenger,
            const VkAllocationCallbacks* pAllocator);
    };
}
