#pragma once
#include <vulkan/vulkan.h>
#include <vector>

#include "LightData.hpp"
#include "../vulkanContext/VulkanContext.hpp"
#include "../swapChainManager/SwapChainManager.hpp"
#include "../renderPipelineManager/RenderPipelineManager.hpp"
#include "../descriptorManager/DescriptorManager.h"
#include "../descriptorManager/buffers/LightBufferData.hpp"

namespace vks {
#ifdef ENABLE_IMGUI
    class ImguiManager;
#endif
    class MeshDescriptor;
    struct NodeDescriptorStruct;


    struct RenderCommand
    {
        boost::uuids::uuid modelId;
        glm::mat4 transform;
    };

    class RenderManager {

private:
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
        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;  // Double buffering

        RenderManager(VulkanContext* context,
                     SwapChainManager* swapChain,
                     RenderPipelineManager* pipelineManager,
                     DescriptorManager* descriptorManager);
        ~RenderManager();

        void initialize();
        #ifdef ENABLE_IMGUI
        void initializeImgui(ImguiManager* manager);
        #endif
        void cleanup();

        // Core rendering functions
        void submitRenderCommand(boost::uuids::uuid modelId, glm::mat4 transform);
        void submitLightCommand(gfx::DirectionalLightData data, glm::mat4 transform); // Prob will pack transform later on for optimization but for now IDK enough
        void submitLightCommand(gfx::PointLightData data, glm::mat4 transform);
        void submitLightCommand(gfx::SpotLightData data, glm::mat4 transform);

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
        std::vector<DirectionalLightBufferData> directionalLightQueue;
        std::vector<PointLightBufferData> pointLightQueue;
        std::vector<SpotLightBufferData> spotLightQueue;

        // Core Vulkan components
        VulkanContext* context;
        SwapChainManager* swapChain;
        RenderPipelineManager* pipelineManager;
        DescriptorManager* descriptorManager;

#ifdef ENABLE_IMGUI
        ImguiManager* imguiManager = nullptr;
#endif

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
        void renderNode(vks::NodeDescriptorStruct* mainNode, VkCommandBuffer commandBuffer, const glm::mat4 matrix);
    };

} // namespace vks