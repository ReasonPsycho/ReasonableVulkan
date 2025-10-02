
#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include "../base/VulkanDevice.h"

struct QueueFamilyIndices {
    uint32_t graphics = UINT32_MAX;
    uint32_t present = UINT32_MAX;
    uint32_t transfer = UINT32_MAX;

    bool isComplete() const {
        return graphics != UINT32_MAX &&
               present != UINT32_MAX &&
               transfer != UINT32_MAX;
    }
};


enum class QueueType {
    Graphics,
    Transfer
};

namespace vks {
    class VulkanContext {
    public:
        VulkanContext();
        ~VulkanContext();

        // Getters
        VkInstance getInstance() const { return instance; }
        VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }
        VkDevice getDevice() const { return device; }
        QueueFamilyIndices getQueueFamilyIndices() const { return queueFamilyIndices; }

        VkQueue getGraphicsQueue() const { return graphicsQueue; }
        VkQueue getTransferQueue() const { return transferQueue; }
        VkCommandPool getTransferCommandPool() const { return transferCommandPool; }
        VkCommandPool getGraphicsCommandPool() const { return graphicsCommandPool; }

        void createBuffer(
                    VkDeviceSize size,
                    VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties,
                    VkBuffer& buffer,
                    VkDeviceMemory& bufferMemory) const;

        void copyBuffer(
            VkBuffer srcBuffer,
            VkBuffer dstBuffer,
            VkDeviceSize size,
            QueueType queueType = QueueType::Transfer) const;


        VkCommandBuffer beginSingleTimeCommands(QueueType queueType = QueueType::Transfer) const;
        void endSingleTimeCommands(VkCommandBuffer commandBuffer, QueueType queueType = QueueType::Transfer) const;

        // Helper method to get the appropriate queue
        VkQueue getQueue(QueueType queueType) const {
            return queueType == QueueType::Graphics ? graphicsQueue : transferQueue;
        }

        // Helper method to get the appropriate command pool
        VkCommandPool getCommandPool(QueueType queueType) const {
            return queueType == QueueType::Graphics ? graphicsCommandPool : transferCommandPool;
        }

        const VkPhysicalDeviceProperties& getDeviceProperties() const { return deviceProperties; }
        const VkPhysicalDeviceFeatures& getDeviceFeatures() const { return deviceFeatures; }
        const VkPhysicalDeviceMemoryProperties& getMemoryProperties() const { return memoryProperties; }

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

    private:
        // Instance related
        VkInstance instance{VK_NULL_HANDLE};
        VkDebugUtilsMessengerEXT debugMessenger{VK_NULL_HANDLE};
        std::vector<const char*> enabledLayers;
        std::vector<const char*> enabledExtensions;

        // Device related
        VkPhysicalDevice physicalDevice{VK_NULL_HANDLE};
        VkDevice device{VK_NULL_HANDLE};
        VkQueue graphicsQueue{VK_NULL_HANDLE};
        VkQueue transferQueue{VK_NULL_HANDLE};
        VkCommandPool graphicsCommandPool{VK_NULL_HANDLE};
        VkCommandPool transferCommandPool{VK_NULL_HANDLE};

        // Device properties
        VkPhysicalDeviceProperties deviceProperties{};
        VkPhysicalDeviceFeatures deviceFeatures{};
        VkPhysicalDeviceMemoryProperties memoryProperties{};


        VkResult createInstance();
        void setupDebugMessenger();
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createQueues();
        void createCommandPools();

        QueueFamilyIndices queueFamilyIndices;

        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
        bool checkDeviceExtensionSupport(VkPhysicalDevice device);

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
