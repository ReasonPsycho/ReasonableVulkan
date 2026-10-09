#pragma once
#include <vulkan/vulkan.h>
#include <vector>

#include "Handle.hpp"
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
        gfx::MeshHandle meshHandle;
        gfx::MaterialHandle materialHandle;
        gfx::ShaderProgramHandle renderProgramHandle;
        glm::mat4 transform;
    };

    struct SkyboxRenderCommand
    {
        uint32_t cameraIndex;
        gfx::MaterialHandle skyboxMaterialHandle;
        gfx::ShaderProgramHandle renderProgramHandle;
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

        void initialize(gfx::ShaderProgramHandle pbrShader, gfx::ShaderProgramHandle skyboxShader, gfx::ShaderProgramHandle shadowShader, gfx::ShaderProgramHandle cubeShadowShader, gfx::ShaderProgramHandle raycastShader);
        void initialize(boost::uuids::uuid pbrShaderId, boost::uuids::uuid skyboxShaderId, boost::uuids::uuid shadowShaderId, boost::uuids::uuid cubeShadowShaderId, boost::uuids::uuid raycastShaderId);
        #ifdef ENABLE_IMGUI
        void initializeImgui(ImguiManager* manager);
        #endif
        void cleanup();

        // Core rendering functions
        void drawModel(uint32_t cameraIndex, gfx::MeshHandle meshHandle, gfx::MaterialHandle materialHandle, gfx::ShaderProgramHandle renderProgramHandle, const glm::mat4& transform);
        void drawSkybox(uint32_t cameraIndex, gfx::MaterialHandle skyboxMaterialHandle, gfx::ShaderProgramHandle renderProgramHandle);
        void submitRenderCommand(uint32_t cameraIndex, gfx::MeshHandle meshHandle, gfx::MaterialHandle materialHandle, gfx::ShaderProgramHandle renderProgramHandle, glm::mat4 transform);
        void submitSkyboxRenderCommand(uint32_t cameraIndex, gfx::MaterialHandle skyboxMaterialHandle, gfx::ShaderProgramHandle renderProgramHandle);
        void submitLightCommand(gfx::DirectionalLightData data, glm::mat4 transform); // Prob will pack transform later on for optimization but for now IDK enough
        void submitLightCommand(gfx::PointLightData data, glm::mat4 transform);
        void submitLightCommand(gfx::SpotLightData data, glm::mat4 transform);

        void beginFrame();
        void renderFrame();
        void endFrame();
        void waitIdle();
        void updateSyncObjects();

        bool renderAndCaptureModelThumbnail(
            const boost::uuids::uuid& modelId,
            const std::string& outputPath,
            int targetWidth,
            int targetHeight,
            const glm::mat4& viewMatrix,
            const glm::mat4& projMatrix,
            const glm::vec3& camPos,
            const glm::vec3& lightDir,
            const glm::vec3& lightColor,
            float lightIntensity);

        bool renderAndCaptureMaterialThumbnail(
            const boost::uuids::uuid& materialId,
            const boost::uuids::uuid& sphereModelId,
            const std::string& outputPath,
            int targetWidth,
            int targetHeight,
            const glm::mat4& viewMatrix,
            const glm::mat4& projMatrix,
            const glm::vec3& camPos,
            const glm::vec3& lightDir,
            const glm::vec3& lightColor,
            float lightIntensity);

        size_t getCurrentFrame() const { return currentFrame; }
        void setActiveCameraCount(uint32_t count) { activeCameraCount = std::max(activeCameraCount, count); }


        // Command buffer management
        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);

        // Resource update functions
        void updateUniformBuffers(uint32_t currentImage);


        // Command recording
        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    private:
        void bindPipelineDescriptors(VkCommandBuffer commandBuffer, gfx::ShaderProgramHandle renderProgramHandle, uint32_t imageIndex, const std::vector<ShaderDefinesEnum>& defines);
        void bindMeshDescriptors(VkCommandBuffer commandBuffer, gfx::ShaderProgramHandle renderProgramHandle, MeshDescriptor* mesh, const std::vector<ShaderDefinesEnum>& defines, MaterialDescriptor* materialOverride = nullptr);

        gfx::ShaderProgramHandle pbrShaderHandle{gfx::ShaderProgramHandle::invalid()};
        gfx::ShaderProgramHandle skyboxShaderHandle{gfx::ShaderProgramHandle::invalid()};
        gfx::ShaderProgramHandle shadowShaderHandle{gfx::ShaderProgramHandle::invalid()};
        gfx::ShaderProgramHandle cubeShadowShaderHandle{gfx::ShaderProgramHandle::invalid()};
        gfx::ShaderProgramHandle raycastShaderHandle{gfx::ShaderProgramHandle::invalid()};

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
        void renderNode(vks::NodeDescriptorStruct* mainNode, VkCommandBuffer commandBuffer, const glm::mat4 matrix, gfx::ShaderProgramHandle renderProgramHandle, MaterialDescriptor* materialOverride = nullptr);
        void renderLightNode(vks::NodeDescriptorStruct* mainNode, VkCommandBuffer commandBuffer, const glm::mat4 matrix, gfx::ShaderProgramHandle renderProgramHandle, int lightIndex, int lightType);
    };

} // namespace vks