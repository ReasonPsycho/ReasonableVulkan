#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "../vulkanContext/VulkanContext.hpp"

namespace vks {
    class SwapChainManager {
    public:
        SwapChainManager(VulkanContext* context);
        ~SwapChainManager();

        void createSurface(void* windowHandle);
        void createSwapChain(uint32_t width, uint32_t height);
        void recreateSwapChain(uint32_t width, uint32_t height);

        // Getters
        VkFormat getSwapChainImageFormat() const { return swapChainImageFormat; }
        VkExtent2D getSwapChainExtent() const { return swapChainExtent; }
        const std::vector<VkImageView>& getImageViews() const { return swapChainImageViews; }
        VkSwapchainKHR getSwapChain() const { return swapChain; }

        VkResult acquireNextImage(VkSemaphore imageAvailableSemaphore);
        VkResult queuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore);

        uint32_t getCurrentImageIndex() const { return currentImageIndex; }

        float windowWidth;
        float windowHeight;
    private:
        VulkanContext* context;
        VkSurfaceKHR surface{VK_NULL_HANDLE};
        VkSwapchainKHR swapChain{VK_NULL_HANDLE};

        uint32_t currentImageIndex{UINT32_MAX};  // Initialize to invalid value
        std::vector<VkImage> swapChainImages;
        std::vector<VkImageView> swapChainImageViews;
        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;

        void createImageViews();
        void cleanup();
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height);
    };
}