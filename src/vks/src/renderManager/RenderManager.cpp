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

void RenderManager::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    
    // Create sync objects for each swapchain image
    imageSync.resize(swapChain->getImageViews().size());

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    // Create image available semaphores
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(context->getDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image available semaphore!");
        }
    }

    // Create per-image synchronization objects
    for (auto& sync : imageSync) {
        if (vkCreateSemaphore(context->getDevice(), &semaphoreInfo, nullptr, &sync.renderFinishedSemaphore) != VK_SUCCESS ||
            vkCreateFence(context->getDevice(), &fenceInfo, nullptr, &sync.inFlightFence) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image sync objects!");
        }
    }
}


    void vks::RenderManager::renderNode(vks::NodeDescriptorStruct* mainNode, VkCommandBuffer commandBuffer, RenderCommand& cmd)
{
    for (const auto& node : mainNode->children) {
        glm::mat4 nodeWorldTransform = cmd.transform * node->matrix;

        for (const auto& mesh : node->meshes) {
            VkBuffer vertexBuffers[] = { mesh->vertices.buffer.buffer };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer, mesh->indices.buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

            // First bind the uniform buffer descriptor set
            if (mesh->uniformBuffer.descriptorSet != VK_NULL_HANDLE) {
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineManager->getMeshPipelineLayout(), 0, 1, &mesh->uniformBuffer.descriptorSet, 0, nullptr);
            }

            // Then bind the material descriptor set at set index 1
            auto materialDescriptorSet = mesh->material->descriptorSet;
            if (materialDescriptorSet != VK_NULL_HANDLE) {
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineManager->getMeshPipelineLayout(), 1, 1, &materialDescriptorSet, 0, nullptr);
            }

            vkCmdPushConstants(commandBuffer, pipelineManager->getMeshPipelineLayout(),
                VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &nodeWorldTransform);

            vkCmdDraw(commandBuffer, mesh->vertices.count, mesh->indices.count, 0, 0);
        }
    }
}

void RenderManager::cleanup() {
    vkDeviceWaitIdle(context->getDevice());

    for (auto& semaphore : imageAvailableSemaphores) {
        vkDestroySemaphore(context->getDevice(), semaphore, nullptr);
    }

    for (auto& sync : imageSync) {
        vkDestroySemaphore(context->getDevice(), sync.renderFinishedSemaphore, nullptr);
        vkDestroyFence(context->getDevice(), sync.inFlightFence, nullptr);
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

    void RenderManager::beginFrame() {
    // First wait for the previous frame to complete
    if (currentImageIndex != UINT32_MAX) {
        vkWaitForFences(context->getDevice(), 1, &imageSync[currentImageIndex].inFlightFence, VK_TRUE, UINT64_MAX);
    }

    VkResult result = swapChain->acquireNextImage(imageAvailableSemaphores[currentFrame]);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        throw std::runtime_error("Swap chain out of date!");
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    // Get the newly acquired image index from the swap chain manager
    currentImageIndex = swapChain->getCurrentImageIndex();

    // Reset fence only after we've acquired a new image and know which fence we'll use
    vkResetFences(context->getDevice(), 1, &imageSync[currentImageIndex].inFlightFence);

    // Mark the image as acquired and update its last used frame
    imageSync[currentImageIndex].imageAcquired = true;
    imageSync[currentImageIndex].frameLastUsed = currentFrame;
}

void RenderManager::renderFrame() {
    // Update uniform buffers and record command buffer
    updateUniformBuffers(currentFrame);
    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(commandBuffers[currentFrame], currentImageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = {imageSync[currentImageIndex].renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(context->getGraphicsQueue(), 1, &submitInfo,
                      imageSync[currentImageIndex].inFlightFence) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkResult result = swapChain->queuePresent(context->getGraphicsQueue(),
                                           currentImageIndex,
                                           imageSync[currentImageIndex].renderFinishedSemaphore);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // Handle swapchain recreation
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void RenderManager::endFrame() {
    // No longer need to do anything here as synchronization is handled in beginFrame
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

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}}; // Color clear value
    clearValues[1].depthStencil = {1.0f, 0}; // Depth clear value
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Bind the model pipeline for model rendering
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineManager->getModelPipeline());

    // Set dynamic viewport and scissor BEFORE drawing
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChain->getSwapChainExtent().width);
    viewport.height = static_cast<float>(swapChain->getSwapChainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChain->getSwapChainExtent();
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // Process render queue
    for (auto& cmd : renderQueue) {
        auto modelDescriptor = descriptorManager->getOrLoadResource<ModelDescriptor>(cmd.modelId);
        if (!modelDescriptor) continue;

        renderNode(modelDescriptor->nodes[0], commandBuffer, cmd);
    }

    renderQueue.clear();
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