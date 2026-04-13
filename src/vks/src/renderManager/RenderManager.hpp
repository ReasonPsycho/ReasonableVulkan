#pragma once
#include <vulkan/vulkan.h>
#include <vector>

#include "LightData.hpp"
#include "../vulkanContext/VulkanContext.hpp"
#include "../swapChainManager/SwapChainManager.hpp"
#include "../renderPipelineManager/RenderPipelineManager.hpp"
#include "../descriptorManager/DescriptorManager.h"
#include "../descriptorManager/buffers/LightBufferData.hpp"

#include <glm/glm.hpp>

namespace vks {
#ifdef ENABLE_IMGUI
    class ImguiManager;
#endif
    class MeshDescriptor;
    struct NodeDescriptorStruct;


    struct RenderCommand
    {
        uint32_t cameraIndex;
        boost::uuids::uuid modelId;
        boost::uuids::uuid renderProgramId;
        glm::mat4 transform;
    };

    struct SkyboxRenderCommand
    {
        uint32_t cameraIndex;
        boost::uuids::uuid textureId;
        boost::uuids::uuid renderProgramId;
    };

    class RenderManager {

private:
    struct FrameResource {
        VkCommandBuffer commandBuffer;
    };

    uint32_t currentImageIndex = UINT32_MAX;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;

    std::vector<FrameResource> frameResources;

    public:
        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;  // Double buffering

        RenderManager(VulkanContext* context,
                     SwapChainManager* swapChain,
                     RenderPipelineManager* pipelineManager,
                     DescriptorManager* descriptorManager);
        ~RenderManager();

        void initialize(boost::uuids::uuid pbrShaderId, boost::uuids::uuid skyboxShaderId, boost::uuids::uuid shadowShaderId, boost::uuids::uuid cubeShadowShaderId);
        #ifdef ENABLE_IMGUI
        void initializeImgui(ImguiManager* manager);
        #endif
        void cleanup();

        // Core rendering functions
        void submitRenderCommand(uint32_t cameraIndex, boost::uuids::uuid modelId, boost::uuids::uuid renderProgramId, glm::mat4 transform);
        void submitSkyboxRenderCommand(uint32_t cameraIndex, boost::uuids::uuid textureId, boost::uuids::uuid renderProgramId);
        void submitLightCommand(gfx::DirectionalLightData data, glm::mat4 transform); // Prob will pack transform later on for optimization but for now IDK enough
        void submitLightCommand(gfx::PointLightData data, glm::mat4 transform);
        void submitLightCommand(gfx::SpotLightData data, glm::mat4 transform);

    void beginFrame();
        void renderFrame();
        void endFrame();
        void waitIdle();

        size_t getCurrentFrame() const { return currentFrame; }
        void setActiveCameraCount(uint32_t count) { activeCameraCount = count; }


        // Command buffer management
        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);

        // Resource update functions
        void updateUniformBuffers(uint32_t currentImage);


        // Command recording
        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    private:
        void bindPipelineDescriptors(VkCommandBuffer commandBuffer, boost::uuids::uuid renderProgramId, uint32_t imageIndex, const std::vector<ShaderDefinesEnum>& defines);
        void bindMeshDescriptors(VkCommandBuffer commandBuffer, boost::uuids::uuid renderProgramId, MeshDescriptor* mesh, const std::vector<ShaderDefinesEnum>& defines);

        boost::uuids::uuid pbrShaderId;
        boost::uuids::uuid skyboxShaderId;
        boost::uuids::uuid shadowShaderId;
        boost::uuids::uuid cubeShadowShaderId;

    private:
        std::vector<RenderCommand> renderQueue;
        std::vector<SkyboxRenderCommand> skyboxRenderQueue;
        std::vector<DirectionalLightBufferData> directionalLightQueue;
        std::vector<PointLightBufferData> pointLightQueue;
        std::vector<SpotLightBufferData> spotLightQueue;

        // Core Vulkan components
        VulkanContext* context;
        SwapChainManager* swapChain;
        RenderPipelineManager* pipelineManager;
        DescriptorManager* descriptorManager;

        uint32_t activeCameraCount = 1;

#ifdef ENABLE_IMGUI
        ImguiManager* imguiManager = nullptr;
#endif

        size_t currentFrame = 0;

        // Command buffer management
        void createCommandBuffers();
        void createSyncObjects();

        //Render helper functions
        void renderNode(vks::NodeDescriptorStruct* mainNode, VkCommandBuffer commandBuffer, const glm::mat4 matrix, boost::uuids::uuid renderProgramId);
        void renderLightNode(vks::NodeDescriptorStruct* mainNode, VkCommandBuffer commandBuffer, const glm::mat4 matrix, boost::uuids::uuid renderProgramId, int lightIndex, int lightType);
    };

} // namespace vks