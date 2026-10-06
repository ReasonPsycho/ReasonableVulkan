
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
#include <stb_image.h>
#include <stb_image_write.h>
#include <stb_image_resize2.h>
#include <filesystem>
#include <boost/uuid/uuid_io.hpp>

#include "src/imguiManager/ImguiManager.hpp"
#include "RenderDocManager.hpp"


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

    gfx::MeshHandle VulkanRenderer::loadMesh(boost::uuids::uuid uuid) {
        return descriptorManager->getOrLoadMesh(uuid);
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

    gfx::MeshHandle VulkanRenderer::getMeshHandle(const std::string& lookUpName) {
        return descriptorManager->getOrLoadMesh(lookUpName);
    }

    gfx::ShaderProgramHandle VulkanRenderer::getShaderHandle(const std::string& lookUpName) {
        return descriptorManager->getOrLoadShaderProgram(lookUpName);
    }

    gfx::MaterialHandle VulkanRenderer::getMaterialHandle(const std::string& lookUpName) {
        return descriptorManager->getOrLoadMaterial(lookUpName);
    }

    void VulkanRenderer::drawModel(uint32_t cameraIndex, gfx::MeshHandle meshHandle, gfx::MaterialHandle materialHandle, gfx::ShaderProgramHandle shaderHandle, const glm::mat4& transform) {
        if (!shaderHandle.isValid()){
            shaderHandle = pbrShaderHandle;
        }
        renderManager->drawModel(cameraIndex, meshHandle, materialHandle, shaderHandle, transform);
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
            rd::RenderDocManager::getInstance().onFrameBegin();
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
            rd::RenderDocManager::getInstance().onFrameEnd();
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

    void* VulkanRenderer::getThumbnailTexture(const boost::uuids::uuid& id, const std::string& thumbnailPath)
    {
#if ENABLE_IMGUI
        if (!imguiManager || !descriptorManager || !context) {
            return nullptr;
        }

        auto it = thumbnailCache.find(id);
        if (it != thumbnailCache.end()) {
            return (void*)it->second.descriptorSet;
        }

        std::filesystem::path resolvedPath = thumbnailPath;
        if (resolvedPath.empty() || !std::filesystem::exists(resolvedPath)) {
            std::filesystem::path p1 = "res/.cache/thumbnails/" + boost::uuids::to_string(id) + ".png";
            std::filesystem::path p2 = ".cache/thumbnails/" + boost::uuids::to_string(id) + ".png";
            if (std::filesystem::exists(p1)) {
                resolvedPath = p1;
            } else if (std::filesystem::exists(p2)) {
                resolvedPath = p2;
            } else {
                return nullptr;
            }
        }

        int width = 0, height = 0, channels = 0;
        stbi_set_flip_vertically_on_load(false);
        unsigned char* pixels = stbi_load(resolvedPath.string().c_str(), &width, &height, &channels, 4);
        if (!pixels) {
            return nullptr;
        }

        VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * height * 4;
        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
        context->createBuffer(
            imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingMemory);

        void* data = nullptr;
        vkMapMemory(context->getDevice(), stagingMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, imageSize);
        vkUnmapMemory(context->getDevice(), stagingMemory);
        stbi_image_free(pixels);

        VkImageCreateInfo imageCreateInfo{};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageCreateInfo.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
        imageCreateInfo.mipLevels = 1;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkImage thumbnailImage = VK_NULL_HANDLE;
        if (vkCreateImage(context->getDevice(), &imageCreateInfo, nullptr, &thumbnailImage) != VK_SUCCESS) {
            vkDestroyBuffer(context->getDevice(), stagingBuffer, nullptr);
            vkFreeMemory(context->getDevice(), stagingMemory, nullptr);
            return nullptr;
        }

        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(context->getDevice(), thumbnailImage, &memReqs);

        VkMemoryAllocateInfo memAllocInfo{};
        memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memAllocInfo.allocationSize = memReqs.size;
        memAllocInfo.memoryTypeIndex = context->findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkDeviceMemory thumbnailMemory = VK_NULL_HANDLE;
        if (vkAllocateMemory(context->getDevice(), &memAllocInfo, nullptr, &thumbnailMemory) != VK_SUCCESS) {
            vkDestroyImage(context->getDevice(), thumbnailImage, nullptr);
            vkDestroyBuffer(context->getDevice(), stagingBuffer, nullptr);
            vkFreeMemory(context->getDevice(), stagingMemory, nullptr);
            return nullptr;
        }

        vkBindImageMemory(context->getDevice(), thumbnailImage, thumbnailMemory, 0);

        VkCommandBuffer cmd = context->beginSingleTimeCommands(QueueType::Graphics);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = thumbnailImage;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};

        vkCmdCopyBufferToImage(cmd, stagingBuffer, thumbnailImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        context->endSingleTimeCommands(cmd, QueueType::Graphics);

        vkDestroyBuffer(context->getDevice(), stagingBuffer, nullptr);
        vkFreeMemory(context->getDevice(), stagingMemory, nullptr);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = thumbnailImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView thumbnailView = VK_NULL_HANDLE;
        if (vkCreateImageView(context->getDevice(), &viewInfo, nullptr, &thumbnailView) != VK_SUCCESS) {
            vkDestroyImage(context->getDevice(), thumbnailImage, nullptr);
            vkFreeMemory(context->getDevice(), thumbnailMemory, nullptr);
            return nullptr;
        }

        VkDescriptorSet descriptorSet = imguiManager->addTexture(thumbnailView, descriptorManager->defaultSampler);
        thumbnailCache[id] = {thumbnailImage, thumbnailMemory, thumbnailView, descriptorSet};
        return (void*)descriptorSet;
#else
        return nullptr;
#endif
    }

    bool VulkanRenderer::renderAndCaptureModelThumbnail(
        const boost::uuids::uuid& modelId,
        const std::string& outputPath,
        int targetWidth,
        int targetHeight,
        const glm::mat4& viewMatrix,
        const glm::mat4& projMatrix,
        const glm::vec3& camPos,
        const glm::vec3& lightDir,
        const glm::vec3& lightColor,
        float lightIntensity)
    {
        if (!renderManager) {
            return false;
        }
        return renderManager->renderAndCaptureModelThumbnail(
            modelId, outputPath, targetWidth, targetHeight,
            viewMatrix, projMatrix, camPos,
            lightDir, lightColor, lightIntensity
        );
    }

    bool VulkanRenderer::captureOffscreenImage(uint32_t cameraIndex, const std::string& outputPath, int targetWidth, int targetHeight) {
        waitIdle();

        if (!pipelineManager || !swapChain || !context) {
            return false;
        }

        uint32_t imageIndex = swapChain->getCurrentImageIndex();
        const auto& cameraRes = pipelineManager->cameraResources;
        if (cameraIndex >= cameraRes.size() || imageIndex >= cameraRes[cameraIndex].offscreenTargets.size()) {
            return false;
        }

        VkImage srcImage = cameraRes[cameraIndex].offscreenTargets[imageIndex].image;
        if (srcImage == VK_NULL_HANDLE) {
            return false;
        }

        VkExtent2D extent = swapChain->getSwapChainExtent();
        uint32_t width = extent.width;
        uint32_t height = extent.height;
        if (width == 0 || height == 0) {
            return false;
        }

        VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * height * 4;

        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
        context->createBuffer(
            imageSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingMemory
        );

        VkCommandBuffer cmd = context->beginSingleTimeCommands(QueueType::Graphics);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = srcImage;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};

        vkCmdCopyImageToBuffer(cmd, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer, 1, &region);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        context->endSingleTimeCommands(cmd, QueueType::Graphics);

        void* mappedData = nullptr;
        if (vkMapMemory(context->getDevice(), stagingMemory, 0, imageSize, 0, &mappedData) == VK_SUCCESS) {
            std::filesystem::path outPath(outputPath);
            std::error_code ec;
            std::filesystem::create_directories(outPath.parent_path(), ec);

            std::vector<uint8_t> thumbPixels(targetWidth * targetHeight * 4);
            stbir_resize_uint8_linear(
                reinterpret_cast<const unsigned char*>(mappedData),
                width,
                height,
                0,
                thumbPixels.data(),
                targetWidth,
                targetHeight,
                0,
                STBIR_RGBA
            );

            stbi_write_png(outputPath.c_str(), targetWidth, targetHeight, 4, thumbPixels.data(), targetWidth * 4);
            vkUnmapMemory(context->getDevice(), stagingMemory);

            vkDestroyBuffer(context->getDevice(), stagingBuffer, nullptr);
            vkFreeMemory(context->getDevice(), stagingMemory, nullptr);
            return true;
        }

        vkDestroyBuffer(context->getDevice(), stagingBuffer, nullptr);
        vkFreeMemory(context->getDevice(), stagingMemory, nullptr);
        return false;
    }

    void VulkanRenderer::cleanup() {
        waitIdle();

#if ENABLE_IMGUI
        for (auto& [thumbId, res] : thumbnailCache) {
            if (res.descriptorSet != VK_NULL_HANDLE && imguiManager) {
                imguiManager->removeTexture(res.descriptorSet);
            }
            if (res.view != VK_NULL_HANDLE) {
                vkDestroyImageView(context->getDevice(), res.view, nullptr);
            }
            if (res.image != VK_NULL_HANDLE) {
                vkDestroyImage(context->getDevice(), res.image, nullptr);
            }
            if (res.memory != VK_NULL_HANDLE) {
                vkFreeMemory(context->getDevice(), res.memory, nullptr);
            }
        }
        thumbnailCache.clear();
#endif

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