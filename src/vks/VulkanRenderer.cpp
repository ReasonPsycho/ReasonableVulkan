
#include "VulkanRenderer.h"

#include <imgui.h>
#include "src/vulkanContext/Vulkancontext.hpp"
#include "src/swapChainManager/SwapChainManager.hpp"
#include "src/descriptorManager/DescriptorManager.h"
#include "src/renderManager/renderManager.hpp"
#include "src/renderPipelineManager/RenderpipelineManager.hpp"
#include <stdexcept>
#include "src/descriptorManager/modelDescriptor/descriptors/shaderDescriptor/ShaderDescriptor.h"
#include <SDL3/SDL_vulkan.h>

#include "src/imguiManager/ImguiManager.hpp"

namespace vks {
    class ModelDescriptor;

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

    void VulkanRenderer::setCameraData(const glm::mat4& projection, const glm::mat4& view)
    {
            descriptorManager->updateSceneUBO(projection, view);
    }

    void VulkanRenderer::loadModel(boost::uuids::uuid uuid) {
        descriptorManager->getOrLoadResource<ModelDescriptor>(uuid);
    }

    void VulkanRenderer::loadShader(boost::uuids::uuid uuid) {
        descriptorManager->getOrLoadResource<ShaderDescriptor>(uuid);
    }

    void VulkanRenderer::drawModel(boost::uuids::uuid uuid, const glm::mat4& transform) {
        renderManager->submitRenderCommand(uuid, transform);
    }


    void VulkanRenderer::beginFrame() {
        if (!minimized)
        {
            renderManager->beginFrame();
        }
    }

    void VulkanRenderer::renderFrame() {
        if (!minimized)
        {
            renderManager->renderFrame();
        }
    }

    void VulkanRenderer::endFrame() {
        if (!minimized)
        {
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
        descriptorManager->initialize();

        // Get descriptor set layouts from descriptor manager
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts = descriptorManager->getAllLayouts();

        pipelineManager->createGraphicsPipeline(descriptorSetLayouts);
        pipelineManager->createDepthResources(swapChain->getSwapChainExtent());
        pipelineManager->createFramebuffers(swapChain->getImageViews(), swapChain->getSwapChainExtent());

        // Initialize render manager
        renderManager->initialize();

#if ENABLE_IMGUI
        imguiManager.get()->initialize(windowHandle);
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

        waitIdle();

        // Recreate swap chain
        swapChain->recreateSwapChain(width, height);

        // Recreate depth resources with new dimensions
        pipelineManager->createDepthResources(swapChain->getSwapChainExtent());

        // Recreate framebuffers
        pipelineManager->createFramebuffers(swapChain->getImageViews(), swapChain->getSwapChainExtent());

#if ENABLE_IMGUI
        // Update ImGui display size
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
#endif
    }
} // namespace vks