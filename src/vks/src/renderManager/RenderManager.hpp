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
    struct NodeDescriptorStruct;


    struct RenderCommand
    {
        boost::uuids::uuid modelId;
        glm::mat4 transform;
    };

    class RenderManager {
private:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;  // Double buffering
    struct FrameResource {
        VkCommandBuffer commandBuffer;
        VkSemaphore imageAvailableSemaphore;
        VkSemaphore renderFinishedSemaphore;
        VkFence inFlightFence;
        bool commandBufferRecorded = false;
    };


        // Track the image index from acquireNextImage
        uint32_t currentAcquiredImageIndex = UINT32_MAX;

        // Track which semaphores are currently in use for each swapchain image
        std::vector<VkSemaphore> currentImageSemaphores;
    std::vector<FrameResource> frameResources;
    std::vector<VkFence> imagesInFlight;

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
        void renderFrame();
        void endFrame();
        void waitIdle();


        // Command buffer management
        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);

        // Resource update functions
        void updateUniformBuffers(uint32_t currentImage);


        // Command recording
        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);


    private:
        std::vector<RenderCommand> renderQueue;

        // Core Vulkan components
        VulkanContext* context;
        SwapChainManager* swapChain;
        RenderPipelineManager* pipelineManager;
        DescriptorManager* descriptorManager;

        struct ImageSyncData {
            VkSemaphore renderFinishedSemaphore;
            VkFence inFlightFence;
            bool imageAcquired = false;
            uint32_t frameLastUsed = UINT32_MAX;  // Track which frame last used this image
        };

        std::vector<ImageSyncData> imageSync;  // Per-swapchain image synchronization
        std::vector<VkSemaphore> imageAvailableSemaphores;
        size_t currentFrame = 0;
        uint32_t currentImageIndex = 0;

        // Command buffer management
        void createCommandBuffers();
        void createSyncObjects();

        //Render helper functions
        void renderNode(vks::NodeDescriptorStruct* mainNode,VkCommandBuffer commandBuffer,RenderCommand& cmd);
    };

} // namespace vks