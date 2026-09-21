
#include "VulkanRenderer.h"
#include <imgui.h>
#include "PlatformInterface.hpp"
#include "src/vulkanContext/Vulkancontext.hpp"
#include "src/swapChainManager/SwapChainManager.hpp"
#include "src/descriptorManager/DescriptorManager.h"
#include "src/renderManager/RenderManager.hpp"
#include "src/renderPipelineManager/RenderPipelineManager.hpp"
#include "src/descriptorManager/modelDescriptor/descriptors/shaderProgramDescriptor/ShaderProgramDescriptor.h"
#include "src/descriptorManager/modelDescriptor/descriptors/textureDescriptor/TextureDescriptor.h"
#include <stdexcept>
#include <SDL3/SDL_vulkan.h>

#include "src/imguiManager/ImguiManager.hpp"


namespace vks {
    class ModelDescriptor;
    class ShaderProgramDescriptor;

    VulkanRenderer::VulkanRenderer(am::AssetManagerInterface* assetManagerInterface) : GraphicsEngine() {
        if (assetManagerInterface == nullptr) {
            throw std::invalid_argument("Asset manager interface cannot be null");
        }

        // Create Vulkan context first as other managers depend on it
        context = std::make_unique<VulkanContext>();

        // Create managers in dependency order
        swapChain = std::make_unique<SwapChainManager>(context.get());
        descriptorManager = std::make_unique<DescriptorManager>(
            assetManagerInterface, context.get()
        );
        pipelineManager = std::make_unique<RenderPipelineManager>(
            context.get(),
            swapChain.get(),
            descriptorManager.get()
        );
        renderManager = std::make_unique<RenderManager>(
            context.get(),
            swapChain.get(),
            pipelineManager.get(),
            descriptorManager.get()
        );
#if ENABLE_IMGUI
        imguiManager = std::make_unique<ImguiManager>(
        context.get(),
        swapChain.get(),
        pipelineManager.get(),
        descriptorManager.get());
        renderManager->initializeImgui(imguiManager.get());
#endif

    }

    VulkanRenderer::~VulkanRenderer() {
        cleanup();
    }

    void VulkanRenderer::setCameraData(uint32_t cameraIndex, const glm::mat4& projection, const glm::mat4& view, const glm::vec3 cameraPos)
    {
        glm::mat4 proj = projection; //Vulkan has inverted Y in clip space compared to OpenGL
        proj[1][1] *= -1.0f;

        descriptorManager->updateSceneUBO(cameraIndex, proj, view, cameraPos);
    }

    void VulkanRenderer::setActiveCameraCount(uint32_t count)
    {
        renderManager->setActiveCameraCount(count);
    }

    gfx::ModelHandle VulkanRenderer::loadModel(boost::uuids::uuid uuid) {
        return descriptorManager->getOrLoadModel(uuid);
    }

    gfx::ShaderProgramHandle VulkanRenderer::loadShader(boost::uuids::uuid uuid) {
        return descriptorManager->getOrLoadShaderProgram(uuid);
    }

    gfx::TextureHandle VulkanRenderer::loadTexture(boost::uuids::uuid uuid) {
        return descriptorManager->getOrLoadTexture(uuid);
    }

    gfx::MaterialHandle VulkanRenderer::loadMaterial(boost::uuids::uuid uuid) {
        return descriptorManager->getOrLoadMaterial(uuid);
    }

    gfx::ModelHandle VulkanRenderer::getModelHandle(const std::string& lookUpName) {
        return descriptorManager->getOrLoadModel(lookUpName);
    }

    gfx::ShaderProgramHandle VulkanRenderer::getShaderHandle(const std::string& lookUpName) {
        return descriptorManager->getOrLoadShaderProgram(lookUpName);
    }

    gfx::MaterialHandle VulkanRenderer::getMaterialHandle(const std::string& lookUpName) {
        return descriptorManager->getOrLoadMaterial(lookUpName);
    }

    void VulkanRenderer::drawModel(uint32_t cameraIndex, gfx::ModelHandle modelHandle, gfx::ShaderProgramHandle shaderHandle, const glm::mat4& transform) {
        if (!shaderHandle.isValid()){
            shaderHandle = pbrShaderHandle;
        }
        renderManager->drawModel(cameraIndex, modelHandle, shaderHandle, transform);
    }

    void VulkanRenderer::drawSkybox(uint32_t cameraIndex, gfx::MaterialHandle materialHandle, gfx::ShaderProgramHandle shaderHandle)
    {
        if (!shaderHandle.isValid()){
            shaderHandle = skyboxShaderHandle;
        }
        renderManager->drawSkybox(cameraIndex, materialHandle, shaderHandle);
    }


    void VulkanRenderer::drawLight(gfx::PointLightData pointLightData, const glm::mat4& transform)
    {
        renderManager->submitLightCommand(pointLightData, transform);
    }

    void VulkanRenderer::drawLight(gfx::SpotLightData spotLightData, const glm::mat4& transform)
    {
        renderManager->submitLightCommand(spotLightData, transform);
    }

    void VulkanRenderer::drawLight(gfx::DirectionalLightData directionalLightData, const glm::mat4& transform)
    {
        renderManager->submitLightCommand(directionalLightData, transform);
    }


    void VulkanRenderer::beginFrame() {
        if (!minimized)
        {
            renderManager->beginFrame();
        }
    }

    void VulkanRenderer::renderFrame() {
        // This is now empty because it should be part of endFrame.
    }

    void VulkanRenderer::endFrame() {
        if (!minimized)
        {
            renderManager->renderFrame();
            renderManager->endFrame();
        }
    }


    void VulkanRenderer::initialize(plt::PlatformInterface* platform, uint32_t width, uint32_t height) {
        if (platform == nullptr) {
            throw std::runtime_error("Platform interface cannot be null");
        }

        platformInterface = platform;
        void* windowHandle = platform->GetNativeWindow();

        if (windowHandle == nullptr) {
            throw std::runtime_error("Window handle cannot be null");
        }

        // Subscribe to window events
        platform->SubscribeToEvent(plt::EventType::WindowResize,
            [this](const void* data) {
                const auto* resizeEvent = static_cast<const plt::WindowResizeEvent*>(data);
                this->handleWindowResize(resizeEvent->width, resizeEvent->height);
            });

        platform->SubscribeToEvent(plt::EventType::WindowMinimize,
            [this](const void* /*data*/) {
                waitIdle();
                minimized = true;
            });

        platform->SubscribeToEvent(plt::EventType::WindowRestored,
            [this](const void* /*data*/) {
                minimized = false;
            });

        // Initialize swap chain
        swapChain->createSurface(windowHandle);
        swapChain->createSwapChain(width, height);

        // Create render pass and pipeline
        pipelineManager->createRenderPass();
        pipelineManager->createShadowRenderPass();
        pipelineManager->createShadowResources();
        pipelineManager->createShadowFramebuffers();
        descriptorManager->initialize();

        // Get descriptor set layouts from descriptor manager
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts = descriptorManager->getAllLayouts();

        // Load shader programs
        pbrShaderHandle = descriptorManager->getOrLoadShaderProgram("pbrShader");
        pipelineManager->createGraphicsPipeline(descriptorManager->getShaderProgram(pbrShaderHandle));

        auto wiremesh = descriptorManager->getOrLoadShaderProgram("wiremeshShader");
        pipelineManager->createGraphicsPipeline(descriptorManager->getShaderProgram(wiremesh));

        auto wiremesh_textured = descriptorManager->getOrLoadShaderProgram("wiremeshTexturedShader");
        pipelineManager->createGraphicsPipeline(descriptorManager->getShaderProgram(wiremesh_textured));

        auto raycast = descriptorManager->getOrLoadShaderProgram("raycastShader");
        pipelineManager->createGraphicsPipeline(descriptorManager->getShaderProgram(raycast));

        skyboxShaderHandle = descriptorManager->getOrLoadShaderProgram("skyboxShader");
        pipelineManager->createGraphicsPipeline(descriptorManager->getShaderProgram(skyboxShaderHandle));

        auto shadowMapPipeline = descriptorManager->getOrLoadShaderProgram("shadowMapShader");
        pipelineManager->createShadowPipeline(descriptorManager->getShaderProgram(shadowMapPipeline));

        auto cubeShadowMapPipeline = descriptorManager->getOrLoadShaderProgram("shadowCubeMapShader");
        pipelineManager->createShadowPipeline(descriptorManager->getShaderProgram(cubeShadowMapPipeline));

        descriptorManager->updateShadowDescriptorSet(
            pipelineManager->directionalShadows.view,
            pipelineManager->pointShadows.view,
            pipelineManager->spotShadows.view
        );

        pipelineManager->createDepthResources(swapChain->getSwapChainExtent());
        pipelineManager->createOffscreenResources(swapChain->getSwapChainExtent());
        pipelineManager->createFramebuffers(swapChain->getSwapChainExtent());

        // Initialize render manager
        renderManager->initialize(pbrShaderHandle, skyboxShaderHandle, shadowMapPipeline, cubeShadowMapPipeline, raycast);

#if ENABLE_IMGUI
        imguiManager.get()->initialize(windowHandle, swapChain->getImageViews());
#endif
    }


    void VulkanRenderer::resize(uint32_t width, uint32_t height) {
        handleWindowResize(width, height);
    }

    glm::uvec2 VulkanRenderer::getExtent() {
        VkExtent2D extent = swapChain->getSwapChainExtent();
        return glm::uvec2(extent.width, extent.height);
    }

   	void* VulkanRenderer::getViewportTexturePointer(uint32_t cameraIndex)
   	{
   #if ENABLE_IMGUI
   		if (imguiManager) {
   			uint32_t lastFrame = (renderManager->getCurrentFrame() + RenderManager::MAX_FRAMES_IN_FLIGHT - 1) % RenderManager::MAX_FRAMES_IN_FLIGHT;
   			return (void*)imguiManager->getTexture(cameraIndex, lastFrame);
   		}
   #endif
   		return nullptr;
   	}

   	void* VulkanRenderer::getViewportTexturePointer()
    {
#if ENABLE_IMGUI
        return (void*)imguiManager.get()->getTexture(swapChain->getCurrentImageIndex());
#else
        return nullptr;
#endif
    }

    void VulkanRenderer::cleanup() {
        waitIdle();

        renderManager.release();
        pipelineManager.release();
        descriptorManager.release();
        swapChain.release();
        context.release();
    }

    void VulkanRenderer::waitIdle() {
        renderManager->waitIdle();
    }

    void VulkanRenderer::handleWindowResize(uint32_t width, uint32_t height) {
        if (width == 0 || height == 0) {
            throw std::invalid_argument("Window dimensions cannot be zero");
        }

        VkExtent2D currentExtent = swapChain->getSwapChainExtent();
        if (currentExtent.width == width && currentExtent.height == height) {
            return;
        }

        waitIdle();

        // Recreate swap chain
        swapChain->recreateSwapChain(width, height);
        VkExtent2D newExtent = swapChain->getSwapChainExtent();

        renderManager->updateSyncObjects();

        // Recreate depth resources with new dimensions
        pipelineManager->createDepthResources(newExtent);
        pipelineManager->createOffscreenResources(newExtent);

        // Recreate framebuffers
        pipelineManager->createFramebuffers(newExtent);

#if ENABLE_IMGUI
        imguiManager.get()->createFramebuffers(swapChain->getImageViews());
        imguiManager.get()->createDescriptorSets(swapChain->getImageViews());
        // Update ImGui display size
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
#endif

        waitIdle();


    }
} // namespace vks