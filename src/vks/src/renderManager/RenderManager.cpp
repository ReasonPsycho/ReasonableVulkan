#include "RenderManager.hpp"
#include <stdexcept>

#include "../descriptorManager/modelDescriptor/ModelDescriptor.h"

namespace vks {

RenderManager::RenderManager(VulkanContext* context,
                           SwapChainManager* swapChain,
                           RenderPipelineManager* pipelineManager,
                           DescriptorManager* descriptorManager)
    : context(context)
    , swapChain(swapChain)
    , pipelineManager(pipelineManager)
    , descriptorManager(descriptorManager) {
}

RenderManager::~RenderManager() {
    cleanup();
}

void RenderManager::initialize() {
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
}

void RenderManager::cleanup() {
    vkDeviceWaitIdle(context->getDevice());

    // Cleanup synchronization objects
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(context->getDevice(), renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(context->getDevice(), imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(context->getDevice(), inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(context->getDevice(), commandPool, nullptr);
}

void RenderManager::submitRenderCommand(boost::uuids::uuid modelId, glm::mat4 transform)
{
    renderQueue.push_back(RenderCommand{modelId, transform});
}

void RenderManager::createCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = 0; // TODO: Get from context

    if (vkCreateCommandPool(context->getDevice(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool!");
    }
}

void RenderManager::createCommandBuffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(context->getDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers!");
    }
}

void RenderManager::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.resize(swapChain->getImageViews().size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(context->getDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(context->getDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(context->getDevice(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create synchronization objects!");
        }
    }
}

void RenderManager::drawFrame() {
    prepareFrame();

    // Here you would update any dynamic resources
    updateUniformBuffers(currentFrame);

    // Any additional frame-specific updates would go here

    submitFrame();
}


// Add these function implementations:

    void RenderManager::beginFrame() {    vkWaitForFences(context->getDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    // Try to acquire the next image
    currentImageIndex = swapChain->acquireNextImage(imageAvailableSemaphores[currentFrame]);

    // Check if a previous frame is using this image
    if (imagesInFlight[currentImageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(context->getDevice(), 1, &imagesInFlight[currentImageIndex], VK_TRUE, UINT64_MAX);
    }
    imagesInFlight[currentImageIndex] = inFlightFences[currentFrame];

    // Reset and record command buffer
    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(commandBuffers[currentFrame], currentImageIndex);
}

void RenderManager::endFrame() {
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

    if (vkQueueSubmit(context->getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    // Present the image
    swapChain->queuePresent(context->getGraphicsQueue(), currentImageIndex, renderFinishedSemaphores[currentFrame]);

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void RenderManager::prepareFrame() {
    // Called at the start of each frame
    beginFrame();
}

void RenderManager::submitFrame() {
    // Called at the end of each frame
    endFrame();
}

void RenderManager::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = pipelineManager->getRenderPass();
    renderPassInfo.framebuffer = pipelineManager->getFramebuffer(imageIndex);
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChain->getSwapChainExtent();

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;


    // Bind the model pipeline for model rendering
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineManager->getModelPipeline());

    // Process render queue
    for (const auto& cmd : renderQueue) {
        auto modelDescriptor = descriptorManager->getOrLoadResource<ModelDescriptor>(cmd.modelId);
        if (!modelDescriptor) continue;

        // Traverse through model nodes
        for (const auto& node : modelDescriptor->nodes) {
            // Calculate world transform for this node
            glm::mat4 nodeWorldTransform = cmd.transform * node->matrix;

            // For each mesh in the node
            for (const auto& mesh : node->meshes) {
                // Bind vertex/index buffers
                VkBuffer vertexBuffers[] = { mesh->vertices.buffer.buffer };
                VkDeviceSize offsets[] = { 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
                vkCmdBindIndexBuffer(commandBuffer, mesh->indices.buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

                    // Bind material descriptor set
                    auto materialDescriptorSet = mesh->material->descriptorSet;
                    if (materialDescriptorSet != VK_NULL_HANDLE) {
                        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineManager->getPipelineLayout(), 0, 1, &materialDescriptorSet, 0, nullptr);
                    }

                    // Push the world transform as a push constant
                    vkCmdPushConstants(commandBuffer, pipelineManager->getPipelineLayout(),
                        VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &nodeWorldTransform);

                    vkCmdDraw(commandBuffer,mesh->vertices.count,mesh->indices.count,0,0);
            }
        }
    }

    // Switch to skybox pipeline for skybox rendering
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineManager->getSkyboxPipeline());

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer!");
    }
}

void RenderManager::updateUniformBuffers(uint32_t currentImage) {
    // Update uniform buffers here
}

VkCommandBuffer RenderManager::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(context->getDevice(), &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void RenderManager::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(context->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(context->getGraphicsQueue());

    vkFreeCommandBuffers(context->getDevice(), commandPool, 1, &commandBuffer);
}

void RenderManager::waitIdle() {
    vkDeviceWaitIdle(context->getDevice());
}

} // namespace vks