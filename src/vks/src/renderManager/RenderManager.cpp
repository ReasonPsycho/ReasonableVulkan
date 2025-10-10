#include "RenderManager.hpp"

#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <stdexcept>

#include "../descriptorManager/modelDescriptor/ModelDescriptor.h"


#ifdef ENABLE_IMGUI
#include "../imguiManager/ImguiManager.hpp"
#endif

namespace vks {

RenderManager::RenderManager(VulkanContext* context,
                           SwapChainManager* swapChain,
                           RenderPipelineManager* pipelineManager,
                           DescriptorManager* descriptorManager)
    : context(context)
      , swapChain(swapChain)
      , pipelineManager(pipelineManager)
      , descriptorManager(descriptorManager)
{
}

RenderManager::~RenderManager() {
    cleanup();
}

void RenderManager::initialize() {
    createCommandBuffers();
    createSyncObjects();
}

#ifdef ENABLE_IMGUI
void RenderManager::initializeImgui(ImguiManager* manager)
{
    imguiManager = manager;
}
#endif

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


    void vks::RenderManager::renderNode(vks::NodeDescriptorStruct* mainNode, VkCommandBuffer commandBuffer, const glm::mat4 matrix)
{
    for (const auto& node : mainNode->children) {
        glm::mat4 nodeWorldTransform = matrix * node->matrix;
        for (const auto& mesh : node->meshes) {
            VkBuffer vertexBuffers[] = { mesh->vertices.buffer.buffer };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer, mesh->indices.buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

            // First bind the uniform buffer descriptor set
            if (mesh->uniformBuffer.descriptorSet != VK_NULL_HANDLE) {
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineManager->getMeshPipelineLayout(), 2, 1, &mesh->uniformBuffer.descriptorSet, 0, nullptr);
            }

            // Then bind the material descriptor set at set index 1
            auto materialDescriptorSet = mesh->material->descriptorSet;
            if (materialDescriptorSet != VK_NULL_HANDLE) {
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineManager->getMeshPipelineLayout(), 1, 1, &materialDescriptorSet, 0, nullptr);
            }

            vkCmdPushConstants(commandBuffer, pipelineManager->getMeshPipelineLayout(),
                VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &nodeWorldTransform);

            vkCmdDrawIndexed(commandBuffer, mesh->indices.count, 1, 0, 0, 0);

        }
        renderNode(node, commandBuffer, matrix);
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

    vkDestroyCommandPool(context->getDevice(), context->getGraphicsCommandPool(), nullptr);
}

void RenderManager::submitRenderCommand(boost::uuids::uuid modelId, glm::mat4 transform)
{
    renderQueue.push_back(RenderCommand{modelId, transform});
}

    void RenderManager::createCommandBuffers() {
    frameResources.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = context->getGraphicsCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

    std::vector<VkCommandBuffer> commandBuffers(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateCommandBuffers(context->getDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers!");
    }

    // Assign the command buffers to frame resources
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        frameResources[i].commandBuffer = commandBuffers[i];
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

#ifdef ENABLE_IMGUI
    imguiManager->imguiBeginFrame();
#endif



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
    vkResetCommandBuffer(frameResources[currentFrame].commandBuffer, 0);

    #ifdef ENABLE_IMGUI
        imguiManager->imguiEndFrame();
    #endif

    recordCommandBuffer(frameResources[currentFrame].commandBuffer, currentImageIndex);



    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &frameResources[currentFrame].commandBuffer;

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

}


    void RenderManager::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }

    // Add memory barrier for UBO before using it
    VkBufferMemoryBarrier bufferBarrier{};
    bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    bufferBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;  // Direct from host write
    bufferBarrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
    bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufferBarrier.buffer = descriptorManager->sceneUBO.buffer.buffer;
    bufferBarrier.offset = 0;
    bufferBarrier.size = sizeof(SceneUBO::UniformBlock);

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_HOST_BIT,              // Direct from host
        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,     // To vertex shader
        0,
        0, nullptr,
        1, &bufferBarrier,
        0, nullptr
    );

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

    vkCmdBindDescriptorSets(commandBuffer,
    VK_PIPELINE_BIND_POINT_GRAPHICS,
    pipelineManager->getMeshPipelineLayout(),    // Your pipeline layout
    0,                 // First set
    1,                 // Number of sets
    &descriptorManager->sceneUBO.buffer.descriptorSet,
    0,
    nullptr);

    // Process render queue
    for (auto& cmd : renderQueue) {
        auto modelDescriptor = descriptorManager->getOrLoadResource<ModelDescriptor>(cmd.modelId);
        if (!modelDescriptor) continue;

        renderNode(modelDescriptor->nodes[0], commandBuffer, cmd.transform);
    }

    renderQueue.clear();
    // Switch to skybox pipeline for skybox rendering
    //vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineManager->getSkyboxPipeline());

    vkCmdEndRenderPass(commandBuffer);



#ifdef ENABLE_IMGUI
    imguiManager->imguiRenderFrame(commandBuffer,imageIndex);
#endif

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer!");
    }
}

void RenderManager::updateUniformBuffers(uint32_t currentImage) {

}

VkCommandBuffer RenderManager::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = context->getGraphicsCommandPool();
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

    vkFreeCommandBuffers(context->getDevice(), context->getGraphicsCommandPool(), 1, &commandBuffer);
}

void RenderManager::waitIdle() {
    vkDeviceWaitIdle(context->getDevice());
}

} // namespace vks