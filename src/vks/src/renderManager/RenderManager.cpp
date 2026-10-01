#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "RenderManager.hpp"
#include <algorithm>
#include <stdexcept>
#include "../descriptorManager/modelDescriptor/ModelDescriptor.h"

#ifdef ENABLE_IMGUI
#include "../imguiManager/ImguiManager.hpp"
#endif

#include <boost/uuid/nil_generator.hpp>

#include "../descriptorManager/buffers/LightModelPushConstant.hpp"
#include "../descriptorManager/buffers/ModelPushConstant.hpp"

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

void RenderManager::initialize(gfx::ShaderProgramHandle pbrShader, gfx::ShaderProgramHandle skyboxShader, gfx::ShaderProgramHandle shadowShader, gfx::ShaderProgramHandle cubeShadowShader, gfx::ShaderProgramHandle raycastShader) {
    this->pbrShaderHandle = pbrShader;
    this->skyboxShaderHandle = skyboxShader;
    this->shadowShaderHandle = shadowShader;
    this->cubeShadowShaderHandle = cubeShadowShader;
    this->raycastShaderHandle = raycastShader;
    createCommandBuffers();
    createSyncObjects();

    // Load a default box model for skybox rendering
    descriptorManager->getOrLoadResource<ModelDescriptor>("boxModel");
}

void RenderManager::initialize(boost::uuids::uuid pbrShaderId, boost::uuids::uuid skyboxShaderId, boost::uuids::uuid shadowShaderId, boost::uuids::uuid cubeShadowShaderId, boost::uuids::uuid raycastShaderId) {
    auto pbrHandle = descriptorManager->getOrLoadShaderProgram(pbrShaderId);
    auto skyboxHandle = descriptorManager->getOrLoadShaderProgram(skyboxShaderId);
    auto shadowHandle = descriptorManager->getOrLoadShaderProgram(shadowShaderId);
    auto cubeShadowHandle = descriptorManager->getOrLoadShaderProgram(cubeShadowShaderId);
    auto raycastHandle = descriptorManager->getOrLoadShaderProgram(raycastShaderId);
    initialize(pbrHandle, skyboxHandle, shadowHandle, cubeShadowHandle, raycastHandle);
}

#ifdef ENABLE_IMGUI
void RenderManager::initializeImgui(ImguiManager* manager)
{
    imguiManager = manager;
}
#endif

void RenderManager::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.assign(swapChain->getImageViews().size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(context->getDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(context->getDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(context->getDevice(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}


void RenderManager::renderNode(NodeDescriptorStruct* mainNode, VkCommandBuffer commandBuffer, const glm::mat4 matrix, gfx::ShaderProgramHandle renderProgramHandle)
{
    auto shaderProgramDescriptor = descriptorManager->getShaderProgram(renderProgramHandle);
    if (!shaderProgramDescriptor) return;
    const auto& defines = shaderProgramDescriptor->getDefines();

   bool hasModelPushConstants = std::find(defines.begin(), defines.end(), ShaderDefinesEnum::MODEL_PC_GLSL) != defines.end();

    for (const auto& node : mainNode->children) {
        glm::mat4 nodeWorldTransform = matrix * node->matrix;
        
        if (hasModelPushConstants) {
            ModelPushConstant push_m;
            push_m.model = nodeWorldTransform;
            vkCmdPushConstants(
                commandBuffer,
                pipelineManager->getPipelineLayout(renderProgramHandle),
                VK_SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof(ModelPushConstant),
                &push_m
            );
        }

        for (const auto& mesh : node->meshes) {
            bindMeshDescriptors(commandBuffer, renderProgramHandle, mesh, defines);
            vkCmdDrawIndexed(commandBuffer, mesh->indices.count, 1, 0, 0, 0);
        }
        renderNode(node, commandBuffer, matrix, renderProgramHandle);
    }
}

void RenderManager::renderLightNode(NodeDescriptorStruct* mainNode, VkCommandBuffer commandBuffer, const glm::mat4 matrix, gfx::ShaderProgramHandle renderProgramHandle, int lightIndex, int lightType)
{
    auto shaderProgramDescriptor = descriptorManager->getShaderProgram(renderProgramHandle);
    if (!shaderProgramDescriptor) return;
    const auto& defines = shaderProgramDescriptor->getDefines();

    bool hasLightModelPushConstants = std::find(defines.begin(), defines.end(), ShaderDefinesEnum::LIGHT_MODEL_PC_GLSL) != defines.end();

    for (const auto& node : mainNode->children) {
        glm::mat4 nodeWorldTransform = matrix * node->matrix;

        if (hasLightModelPushConstants) {
            LightModelPushConstant push_lm;
            push_lm.model = nodeWorldTransform;
            push_lm.lightIndex = lightIndex;
            push_lm.lightType = lightType;

            vkCmdPushConstants(
                commandBuffer,
                pipelineManager->getPipelineLayout(renderProgramHandle),
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(LightModelPushConstant),
                &push_lm
            );
        }

        for (const auto& mesh : node->meshes) {
            bindMeshDescriptors(commandBuffer, renderProgramHandle, mesh, defines);
            vkCmdDrawIndexed(commandBuffer, mesh->indices.count, 1, 0, 0, 0);
        }
        renderLightNode(node, commandBuffer, matrix, renderProgramHandle, lightIndex, lightType);
    }
}

void RenderManager::cleanup() {
    vkDeviceWaitIdle(context->getDevice());

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (i < renderFinishedSemaphores.size()) vkDestroySemaphore(context->getDevice(), renderFinishedSemaphores[i], nullptr);
        if (i < imageAvailableSemaphores.size()) vkDestroySemaphore(context->getDevice(), imageAvailableSemaphores[i], nullptr);
        if (i < inFlightFences.size()) vkDestroyFence(context->getDevice(), inFlightFences[i], nullptr);
    }
    
    vkDestroyCommandPool(context->getDevice(), context->getGraphicsCommandPool(), nullptr);
}

void RenderManager::drawModel(uint32_t cameraIndex, gfx::ModelHandle modelHandle, gfx::ShaderProgramHandle renderProgramHandle, const glm::mat4& transform)
{
    renderQueue.push_back(RenderCommand{cameraIndex, modelHandle, renderProgramHandle, transform});
}

void RenderManager::drawSkybox(uint32_t cameraIndex, gfx::MaterialHandle skyboxMaterialHandle, gfx::ShaderProgramHandle renderProgramHandle)
{
    skyboxRenderQueue.push_back(SkyboxRenderCommand{cameraIndex, skyboxMaterialHandle, renderProgramHandle});
}

void RenderManager::submitRenderCommand(uint32_t cameraIndex, gfx::ModelHandle modelHandle, gfx::ShaderProgramHandle renderProgramHandle, glm::mat4 transform)
{
    renderQueue.push_back(RenderCommand{cameraIndex, modelHandle, renderProgramHandle, transform});
}

void RenderManager::submitSkyboxRenderCommand(uint32_t cameraIndex, gfx::MaterialHandle skyboxMaterialHandle, gfx::ShaderProgramHandle renderProgramHandle)
{
    skyboxRenderQueue.push_back(SkyboxRenderCommand{cameraIndex, skyboxMaterialHandle, renderProgramHandle});
}

void RenderManager::submitLightCommand(gfx::DirectionalLightData data, glm::mat4 transform)
{
    DirectionalLightBufferData bufferData{};
    bufferData.direction = glm::normalize(glm::vec3(transform * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
    bufferData.intensity = data.intensity;
    bufferData.color = data.color;
    bufferData.shadowBias = data.shadowBias;
    bufferData.shadowStrength = data.shadowStrength;
    bufferData.shadowMapIndex = -1; // Assigned during rendering
    bufferData.castShadows = data.castShadows;

    directionalLightQueue.push_back(bufferData);
}

    void RenderManager::submitLightCommand(gfx::PointLightData data, glm::mat4 transform)
{
    PointLightBufferData bufferData{};
    bufferData.position = glm::vec3(transform[3]);  // Extract position from transform
    bufferData.intensity = data.intensity;
    bufferData.color = data.color;
    bufferData.radius = data.radius;
    bufferData.falloff = data.falloff;
    bufferData.shadowBias = data.shadowBias;
    bufferData.shadowStrength = data.shadowStrength;
    bufferData.shadowMapIndex = -1; // Assigned during rendering
    bufferData.castShadows = data.castShadows;

    pointLightQueue.push_back(bufferData);
}

    void RenderManager::submitLightCommand(gfx::SpotLightData data, glm::mat4 transform)
{
    SpotLightBufferData bufferData{};
    bufferData.position = glm::vec3(transform[3]);  // Extract position from transform
    bufferData.innerAngle = data.innerAngle;
    bufferData.direction = glm::normalize(glm::vec3(transform * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
    bufferData.outerAngle = data.outerAngle;
    bufferData.color = data.color;
    bufferData.range = data.range;
    bufferData.intensity = data.intensity;
    bufferData.shadowBias = data.shadowBias;
    bufferData.shadowStrength = data.shadowStrength;
    bufferData.shadowMapIndex = -1; // Assigned during rendering
    bufferData.castShadows = data.castShadows;

    spotLightQueue.push_back(bufferData);
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
    activeCameraCount = 0;
    vkWaitForFences(context->getDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    VkResult result = swapChain->acquireNextImage(imageAvailableSemaphores[currentFrame]);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        return; 
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

#ifdef ENABLE_IMGUI
    imguiManager->imguiBeginFrame();
#endif

    currentImageIndex = swapChain->getCurrentImageIndex();

    if (imagesInFlight[currentImageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(context->getDevice(), 1, &imagesInFlight[currentImageIndex], VK_TRUE, UINT64_MAX);
    }
    imagesInFlight[currentImageIndex] = inFlightFences[currentFrame];

    vkResetFences(context->getDevice(), 1, &inFlightFences[currentFrame]);
}

void RenderManager::renderFrame() {
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

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(context->getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        spdlog::error("failed to submit draw command buffer!");
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    #ifdef ENABLE_IMGUI
        imguiManager->imguiRenderPlatformWindows();
    #endif

    VkResult result = swapChain->queuePresent(context->getGraphicsQueue(),
                                           currentImageIndex,
                                           renderFinishedSemaphores[currentFrame]);

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

        // --- Shadow Pass ---
        if (pipelineManager->getShadowRenderPass() != VK_NULL_HANDLE) {
            float farPlane = 25.0f; // TODO: From config
            int directionalShadowCount = 0;
            int pointShadowCount = 0;
            int spotShadowCount = 0;

            for (auto& light : directionalLightQueue) {
                if (light.castShadows && directionalShadowCount < pipelineManager->MAX_DIRECTIONAL_SHADOWS) {
                    glm::vec3 lightDir = light.direction;
                    glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 0.1f, farPlane);
                    lightProjection[1][1] *= -1.0f;
                    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
                    if (glm::abs(glm::dot(lightDir, up)) > 0.999f) {
                        up = glm::vec3(0.0f, 0.0f, 1.0f);
                    }
                    glm::mat4 lightView = glm::lookAt(-lightDir * 10.0f, glm::vec3(0.0f), up);
                    light.lightSpaceMatrix = lightProjection * lightView;
                    light.shadowMapIndex = directionalShadowCount++;

                    // Render to shadow map
                    VkRenderPassBeginInfo renderPassInfo = {};
                    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                    renderPassInfo.renderPass = pipelineManager->getShadowRenderPass();
                    renderPassInfo.framebuffer = pipelineManager->getDirectionalShadowFramebuffer(light.shadowMapIndex);
                    renderPassInfo.renderArea.offset = {0, 0};
                    renderPassInfo.renderArea.extent = {pipelineManager->SHADOWMAP_DIM, pipelineManager->SHADOWMAP_DIM};
                    VkClearValue clearValues[1];
                    clearValues[0].depthStencil = {1.0f, 0};
                    renderPassInfo.clearValueCount = 1;
                    renderPassInfo.pClearValues = clearValues;

                    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

                    VkViewport viewport = base::initializers::viewport((float)pipelineManager->SHADOWMAP_DIM, (float)pipelineManager->SHADOWMAP_DIM, 0.0f, 1.0f);
                    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

                    VkRect2D scissor = base::initializers::rect2D(pipelineManager->SHADOWMAP_DIM, pipelineManager->SHADOWMAP_DIM, 0, 0);
                    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

                    vkCmdSetDepthBias(commandBuffer, 1.25f, 0.0f, 1.75f);

                    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineManager->getPipeline(shadowShaderHandle));

                    auto shadowShaderDescriptor = descriptorManager->getShaderProgram(shadowShaderHandle);
                    if (shadowShaderDescriptor) {
                        bindPipelineDescriptors(commandBuffer, shadowShaderHandle, 0, shadowShaderDescriptor->getDefines());
                    }

                    for (auto& command : renderQueue) {
                        auto modelDescriptor = descriptorManager->getModel(command.modelHandle);
                        if (modelDescriptor) {
                            renderLightNode(modelDescriptor->nodes[0], commandBuffer, command.transform, shadowShaderHandle, light.shadowMapIndex, 0);
                        }
                    }

                    vkCmdEndRenderPass(commandBuffer);
                }
            }

            for (auto& light : pointLightQueue) {
                if (light.castShadows && pointShadowCount < pipelineManager->MAX_POINT_SHADOWS) {
                    light.shadowMapIndex = pointShadowCount++;
                    glm::mat4 shadowProj = glm::perspective(
                        glm::radians(90.0f),
                        1.0f,
                        0.1f,
                        farPlane
                    );


                    light.lightSpaceMatrices[0] = shadowProj * glm::lookAt(light.position, light.position + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));
                    light.lightSpaceMatrices[1] = shadowProj * glm::lookAt(light.position, light.position + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));
                    light.lightSpaceMatrices[2] = shadowProj * glm::lookAt(light.position, light.position + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0));
                    light.lightSpaceMatrices[3] = shadowProj * glm::lookAt(light.position, light.position + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0));
                    light.lightSpaceMatrices[4] = shadowProj * glm::lookAt(light.position, light.position + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0));
                    light.lightSpaceMatrices[5] = shadowProj * glm::lookAt(light.position, light.position + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0));

                    // Render to shadow cube map
                    VkRenderPassBeginInfo renderPassInfo = {};
                    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                    renderPassInfo.renderPass = pipelineManager->getShadowRenderPassMultiview();
                    renderPassInfo.framebuffer = pipelineManager->getPointShadowFramebuffer(light.shadowMapIndex);
                    renderPassInfo.renderArea.offset = {0, 0};
                    renderPassInfo.renderArea.extent = {pipelineManager->SHADOWMAP_DIM, pipelineManager->SHADOWMAP_DIM};
                    VkClearValue clearValues[1];
                    clearValues[0].depthStencil = {1.0f, 0};
                    renderPassInfo.clearValueCount = 1;
                    renderPassInfo.pClearValues = clearValues;

                    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

                    VkViewport viewport = base::initializers::viewport((float)pipelineManager->SHADOWMAP_DIM, (float)pipelineManager->SHADOWMAP_DIM, 0.0f, 1.0f);
                    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

                    VkRect2D scissor = base::initializers::rect2D(pipelineManager->SHADOWMAP_DIM, pipelineManager->SHADOWMAP_DIM, 0, 0);
                    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

                    vkCmdSetDepthBias(commandBuffer, 1.25f, 0.0f, 1.75f);

                    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineManager->getPipeline(cubeShadowShaderHandle));

                    auto cubeShadowShaderDescriptor = descriptorManager->getShaderProgram(cubeShadowShaderHandle);
                    if (cubeShadowShaderDescriptor) {
                        bindPipelineDescriptors(commandBuffer, cubeShadowShaderHandle, 0, cubeShadowShaderDescriptor->getDefines());
                    }

                    for (auto& command : renderQueue) {
                        auto modelDescriptor = descriptorManager->getModel(command.modelHandle);
                        if (modelDescriptor) {
                            renderLightNode(modelDescriptor->nodes[0], commandBuffer, command.transform, cubeShadowShaderHandle, light.shadowMapIndex, 1);
                        }
                    }

                    vkCmdEndRenderPass(commandBuffer);
                }
            }

            for (auto& light : spotLightQueue) {
                if (light.castShadows && spotShadowCount < pipelineManager->MAX_SPOT_SHADOWS) {
                    glm::mat4 shadowProj = glm::perspective(glm::radians(light.outerAngle * 2.0f), 1.0f, 0.1f, light.range);
                    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
                    if (glm::abs(glm::dot(light.direction, up)) > 0.999f) {
                        up = glm::vec3(0.0f, 0.0f, 1.0f);
                    }
                    glm::mat4 shadowView = glm::lookAt(light.position, light.position + light.direction, up);
                    light.lightSpaceMatrix = shadowProj * shadowView;
                    light.shadowMapIndex = spotShadowCount++;

                    // Render to shadow map
                    VkRenderPassBeginInfo renderPassInfo = {};
                    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                    renderPassInfo.renderPass = pipelineManager->getShadowRenderPass();
                    renderPassInfo.framebuffer = pipelineManager->getSpotShadowFramebuffer(light.shadowMapIndex);
                    renderPassInfo.renderArea.offset = {0, 0};
                    renderPassInfo.renderArea.extent = {pipelineManager->SHADOWMAP_DIM, pipelineManager->SHADOWMAP_DIM};
                    VkClearValue clearValues[1];
                    clearValues[0].depthStencil = {1.0f, 0};
                    renderPassInfo.clearValueCount = 1;
                    renderPassInfo.pClearValues = clearValues;

                    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

                    VkViewport viewport = base::initializers::viewport((float)pipelineManager->SHADOWMAP_DIM, (float)pipelineManager->SHADOWMAP_DIM, 0.0f, 1.0f);
                    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

                    VkRect2D scissor = base::initializers::rect2D(pipelineManager->SHADOWMAP_DIM, pipelineManager->SHADOWMAP_DIM, 0, 0);
                    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

                    vkCmdSetDepthBias(commandBuffer, 1.25f, 0.0f, 1.75f);

                    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineManager->getPipeline(shadowShaderHandle));

                    auto spotShadowShaderDescriptor = descriptorManager->getShaderProgram(shadowShaderHandle);
                    if (spotShadowShaderDescriptor) {
                        bindPipelineDescriptors(commandBuffer, shadowShaderHandle, 0, spotShadowShaderDescriptor->getDefines());
                    }

                    for (auto& command : renderQueue) {
                        auto modelDescriptor = descriptorManager->getModel(command.modelHandle);
                        if (modelDescriptor) {
                            renderLightNode(modelDescriptor->nodes[0], commandBuffer, command.transform, shadowShaderHandle, light.shadowMapIndex, 2);
                        }
                    }

                    vkCmdEndRenderPass(commandBuffer);
                }
            }
        }

        // Add memory barrier for UBOs before using them
        std::vector<VkBufferMemoryBarrier> bufferBarriers;
        for (auto& sceneUBO : descriptorManager->sceneUBOs) {
            VkBufferMemoryBarrier bufferBarrier{};
            bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            bufferBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;  // Direct from host write
            bufferBarrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
            bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufferBarrier.buffer = sceneUBO.buffer.buffer;
            bufferBarrier.offset = 0;
            bufferBarrier.size = sizeof(SceneUBO::UniformBlock);
            bufferBarriers.push_back(bufferBarrier);
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_HOST_BIT,              // Direct from host
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,     // To vertex shader
            0,
            0, nullptr,
            static_cast<uint32_t>(bufferBarriers.size()), bufferBarriers.data(),
            0, nullptr
        );

        // Process lights data once per frame
        descriptorManager->updateLightsData(directionalLightQueue, pointLightQueue, spotLightQueue, 25.0f); // TODO: Hardcoded far plane? Or from config?
        directionalLightQueue.clear();
        pointLightQueue.clear();
        spotLightQueue.clear();

        // Sort render queue by renderProgramHandle to minimize pipeline switching (optional, could be per camera)
        std::sort(renderQueue.begin(), renderQueue.end(), [](const RenderCommand& a, const RenderCommand& b) {
            if (a.cameraIndex != b.cameraIndex) return a.cameraIndex < b.cameraIndex;
            return a.renderProgramHandle < b.renderProgramHandle;
        });

        // Loop through all active cameras
        uint32_t cameraCountToRender = std::max(1u, activeCameraCount);
        for (uint32_t i = 0; i < cameraCountToRender; ++i) {
            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = pipelineManager->getRenderPass();
            renderPassInfo.framebuffer = pipelineManager->getFramebuffer(i, imageIndex);
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = swapChain->getSwapChainExtent();

            std::array<VkClearValue, 2> clearValues{};
            clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}}; // Color clear value
            clearValues[1].depthStencil = {1.0f, 0}; // Depth clear value
            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

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

            // Process skybox for this camera
            if (!skyboxRenderQueue.empty()) {
                auto skyboxModel = descriptorManager->getOrLoadResource<ModelDescriptor>("skyboxModel");
                if (skyboxModel && !skyboxModel->meshes.empty()) {
                    auto skyboxMesh = skyboxModel->meshes[0];

                    for (auto& cmd : skyboxRenderQueue) {
                        if (cmd.cameraIndex != i) continue;

                        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineManager->getPipeline(cmd.renderProgramHandle));

                        vkCmdBindDescriptorSets(
                              commandBuffer,
                              VK_PIPELINE_BIND_POINT_GRAPHICS,
                              pipelineManager->getPipelineLayout(cmd.renderProgramHandle),
                              0,                                    // First set index (Set 0)
                              1,                                    // Number of sets
                              &descriptorManager->sceneUBOs[i].buffer.descriptorSet,
                              0, nullptr);


                        // Bind material descriptor set at set index 1
                        auto materialDescriptor = descriptorManager->getMaterial(cmd.skyboxMaterialHandle);
                        if (materialDescriptor) {
                             if (materialDescriptor->descriptorSet == VK_NULL_HANDLE) {
                                  materialDescriptor->setUpDescriptorSet(descriptorManager->skyboxMaterialLayout, descriptorManager->skyboxMaterialPool, descriptorManager->defaultImageInfo, descriptorManager->cubeImageInfo);
                             }

                             auto shaderProgramDescriptor = descriptorManager->getShaderProgram(cmd.renderProgramHandle);
                             if (shaderProgramDescriptor) {
                                 const auto& defines = shaderProgramDescriptor->getDefines();
                                 if (std::find(defines.begin(), defines.end(), ShaderDefinesEnum::MATERIAL_SKYBOX_GLSL) != defines.end()) {
                                     vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        pipelineManager->getPipelineLayout(cmd.renderProgramHandle), 1, 1, &materialDescriptor->descriptorSet, 0, nullptr);
                                 }
                             }
                        }

                        VkBuffer vertexBuffers[] = { skyboxMesh->vertices.buffer.buffer };
                        VkDeviceSize offsets[] = { 0 };
                        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
                        vkCmdBindIndexBuffer(commandBuffer, skyboxMesh->indices.buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

                        ModelPushConstant push_m;
                        push_m.model = glm::mat4(1.0f);
                        vkCmdPushConstants(
                            commandBuffer,
                            pipelineManager->getPipelineLayout(cmd.renderProgramHandle),
                            VK_SHADER_STAGE_VERTEX_BIT,
                            0,
                            sizeof(ModelPushConstant),
                            &push_m
                        );

                        vkCmdDrawIndexed(commandBuffer, skyboxMesh->indices.count, 1, 0, 0, 0);
                    }
                }
            }

            // Process model render queue for this camera
            gfx::ShaderProgramHandle lastProgramHandle = gfx::ShaderProgramHandle::invalid();
            for (auto& cmd : renderQueue) {
                if (cmd.cameraIndex != i) continue;

                auto modelDescriptor = descriptorManager->getModel(cmd.modelHandle);
                if (!modelDescriptor) continue;

                if (cmd.renderProgramHandle != lastProgramHandle) {
                    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineManager->getPipeline(cmd.renderProgramHandle));

                    auto shaderProgramDescriptor = descriptorManager->getShaderProgram(cmd.renderProgramHandle);
                    if (shaderProgramDescriptor) {
                        bindPipelineDescriptors(commandBuffer, cmd.renderProgramHandle, i, shaderProgramDescriptor->getDefines());
                    }

                    lastProgramHandle = cmd.renderProgramHandle;
                }

                renderNode(modelDescriptor->nodes[0], commandBuffer, cmd.transform, cmd.renderProgramHandle);
            }

            vkCmdEndRenderPass(commandBuffer);
        }

        skyboxRenderQueue.clear();
        renderQueue.clear();

#ifdef ENABLE_IMGUI
        imguiManager->imguiRenderFrame(commandBuffer, imageIndex);
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

void RenderManager::updateSyncObjects() {
    imagesInFlight.assign(swapChain->getImageViews().size(), VK_NULL_HANDLE);
}

void RenderManager::bindPipelineDescriptors(VkCommandBuffer commandBuffer, gfx::ShaderProgramHandle renderProgramHandle, uint32_t imageIndex, const std::vector<ShaderDefinesEnum>& defines) {
    bool hasSceneUBO = std::find(defines.begin(), defines.end(), ShaderDefinesEnum::SCENE_UBO_GLSL) != defines.end();
    bool hasLighting = std::find(defines.begin(), defines.end(), ShaderDefinesEnum::LIGHTING_COMMON_GLSL) != defines.end() ||
                       std::find(defines.begin(), defines.end(), ShaderDefinesEnum::LIGHT_MODEL_PC_GLSL) != defines.end();

    if (hasSceneUBO) {
        vkCmdBindDescriptorSets(
              commandBuffer,
              VK_PIPELINE_BIND_POINT_GRAPHICS,
              pipelineManager->getPipelineLayout(renderProgramHandle),
              0,                                    // First set index (Set 0)
              1,                                    // Number of sets
              &descriptorManager->sceneUBOs[imageIndex].buffer.descriptorSet,
              0, nullptr);
    }

    if (hasLighting) {
        vkCmdBindDescriptorSets(
              commandBuffer,
              VK_PIPELINE_BIND_POINT_GRAPHICS,
              pipelineManager->getPipelineLayout(renderProgramHandle),
              3,                                    // Set index 3
              1,                                    // Number of sets
              &descriptorManager->lightInfoUBO.buffer.descriptorSet,
              0, nullptr);
    }
}

void RenderManager::bindMeshDescriptors(VkCommandBuffer commandBuffer, gfx::ShaderProgramHandle renderProgramHandle, MeshDescriptor* mesh, const std::vector<ShaderDefinesEnum>& defines) {
    bool hasMeshUBO = std::find(defines.begin(), defines.end(), ShaderDefinesEnum::VERTEX_IO_GLSL) != defines.end();
    bool hasMaterial = std::find(defines.begin(), defines.end(), ShaderDefinesEnum::MATERIAL_PBR_GLSL) != defines.end();

    VkBuffer vertexBuffers[] = { mesh->vertices.buffer.buffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, mesh->indices.buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    // Bind mesh descriptor set at set index 2
    if (hasMeshUBO && mesh->uniformBuffer.descriptorSet != VK_NULL_HANDLE) {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineManager->getPipelineLayout(renderProgramHandle), 2, 1, &mesh->uniformBuffer.descriptorSet, 0, nullptr);
    }

    // Bind material descriptor set at set index 1
    if (hasMaterial) {
        auto materialDescriptorSet = mesh->material->descriptorSet;
        if (materialDescriptorSet != VK_NULL_HANDLE) {
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipelineManager->getPipelineLayout(renderProgramHandle), 1, 1, &materialDescriptorSet, 0, nullptr);
        }
    }
}

} // namespace vks