
#include "VulkanRenderer.h"
#include "src/vulkanContext/Vulkancontext.hpp"
#include "src/swapChainManager/SwapChainManager.hpp"
#include "src/descriptorManager/DescriptorManager.h"
#include "src/renderManager/renderManager.hpp"
#include "src/renderPipelineManager/RenderpipelineManager.hpp"
#include <stdexcept>
#include "src/descriptorManager/modelDescriptor/descriptors/shaderDescriptor/ShaderDescriptor.h"

namespace vks {
    class ModelDescriptor;


    VulkanRenderer::VulkanRenderer(am::AssetManagerInterface* assetManagerInterface) : GraphicsEngine() {
        if (assetManagerInterface == nullptr)
        {
            throw std::invalid_argument("Asset manager interface cannot be null");
        }
        currentFrame = 0;
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
    }

VulkanRenderer::~VulkanRenderer() {
    cleanup();
}

void VulkanRenderer::loadModel(boost::uuids::uuid uuid)
{
    descriptorManager->getOrLoadResource<ModelDescriptor>(uuid);
}

void VulkanRenderer::loadShader(boost::uuids::uuid uuid)
{
    descriptorManager->getOrLoadResource<ShaderDescriptor>(uuid);
}

void VulkanRenderer::drawModel(boost::uuids::uuid uuid, const glm::mat4& transform)
{
}

void VulkanRenderer::renderFrame()
{
}

void VulkanRenderer::render() {
        if (currentFrame >= MAX_FRAMES_IN_FLIGHT) {
            throw std::runtime_error("currentFrame index out of bounds");
        }

        vkWaitForFences(context->getDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex = swapChain->acquireNextImage(imageAvailableSemaphores[currentFrame]);

    // Reset and record command buffer
    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    renderManager->recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(context->getDevice(), 1, &inFlightFences[currentFrame]);

    if (vkQueueSubmit(context->getGraphicsQueue(), 1, &submitInfo,
                      inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    // Present the image
    swapChain->queuePresent(context->getGraphicsQueue(),
                           imageIndex,
                           renderFinishedSemaphores[currentFrame]);

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::endFrame()
{
    renderManager->endFrame();
}

void VulkanRenderer::initialize(void* windowHandle, uint32_t width, uint32_t height) {
    // Initialize swap chain
    swapChain->createSurface(windowHandle);
    swapChain->createSwapChain(width, height);

    // Create render pass and pipeline
    pipelineManager->createRenderPass();

    // Get descriptor set layouts from descriptor manager
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    // TODO: Get required descriptor set layouts from descriptorManager

    pipelineManager->createGraphicsPipeline(descriptorSetLayouts);

    // Create command pool and buffers
    createCommandPool();
    createCommandBuffers();

    // Create synchronization objects
    createSyncObjects();
}

void VulkanRenderer::cleanup() {
        // Wait for all operations to complete
        waitIdle();

        // Only cleanup sync objects if they were initialized
        if (!imageAvailableSemaphores.empty()) {
            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                vkDestroySemaphore(context->getDevice(), renderFinishedSemaphores[i], nullptr);
                vkDestroySemaphore(context->getDevice(), imageAvailableSemaphores[i], nullptr);
                vkDestroyFence(context->getDevice(), inFlightFences[i], nullptr);
            }
        }

        // Only cleanup command buffers if they were initialized
        if (!commandBuffers.empty()) {
            vkFreeCommandBuffers(context->getDevice(), commandPool,
                             static_cast<uint32_t>(commandBuffers.size()),
                             commandBuffers.data());
        }

        if (commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(context->getDevice(), commandPool, nullptr);
        }

        renderManager.release();
        pipelineManager.release();
        descriptorManager.release();
        swapChain.release();
        context.release();
    }

void VulkanRenderer::waitIdle() {
    vkDeviceWaitIdle(context->getDevice());
}

void VulkanRenderer::handleWindowResize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0)
    {
        throw std::invalid_argument("Window dimensions cannot be zero");
    }

        waitIdle();

    // Recreate swap chain
    swapChain->createSwapChain(width, height);

    // Recreate framebuffers
    pipelineManager->createFramebuffers(swapChain->getImageViews(),
                                       swapChain->getSwapChainExtent());
}

void VulkanRenderer::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(context->getDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(context->getDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(context->getDevice(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects!");
        }
    }
}

void VulkanRenderer::createCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = 0; // TODO: Get from context

    if (vkCreateCommandPool(context->getDevice(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

void VulkanRenderer::createCommandBuffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(context->getDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

} // namespace vks