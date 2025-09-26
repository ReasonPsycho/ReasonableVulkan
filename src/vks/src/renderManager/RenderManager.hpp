#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <glm/matrix.hpp>

#include "../vulkanContext/VulkanContext.hpp"
#include "../swapChainManager/SwapChainManager.hpp"
#include "../renderPipelineManager/RenderPipelineManager.hpp"
#include "../descriptorManager/DescriptorManager.h"

namespace vks {
    class MeshDescriptor;
    struct NodeHandle;


    struct RenderCommand
    {
        boost::uuids::uuid modelId;
        glm::mat4 transform;
    };

    class RenderManager {
    public:
        RenderManager(VulkanContext* context,
                     SwapChainManager* swapChain,
                     RenderPipelineManager* pipelineManager,
                     DescriptorManager* descriptorManager);
        ~RenderManager();

        void initialize();
        void cleanup();

        // Core rendering functions
        void submitRenderCommand(boost::uuids::uuid modelId, glm::mat4 transform);
        void beginFrame();
        void endFrame();
        void drawFrame();
        void waitIdle();


        // Command buffer management
        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);

        // Resource update functions
        void updateUniformBuffers(uint32_t currentImage);


        // Command recording
        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);


    private:
        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
        uint32_t currentFrame = 0;
        uint32_t currentImageIndex= 0;

        std::vector<RenderCommand> renderQueue;

        // Core Vulkan components
        VulkanContext* context;
        SwapChainManager* swapChain;
        RenderPipelineManager* pipelineManager;
        DescriptorManager* descriptorManager;

        // Command buffer resources
        VkCommandPool commandPool{VK_NULL_HANDLE};
        std::vector<VkCommandBuffer> commandBuffers;

        // Synchronization objects
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        std::vector<VkFence> imagesInFlight;

        // Command buffer management
        void createCommandPool();
        void createCommandBuffers();
        void createSyncObjects();

        // Helper functions
        void prepareFrame();
        void submitFrame();
    };

} // namespace vks