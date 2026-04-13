#include <boost/uuid/uuid_io.hpp>
#include "RenderPipelineManager.hpp"
#include "../swapChainManager/SwapChainManager.hpp"
#include <stdexcept>
#include <algorithm>
#include "../descriptorManager/modelDescriptor/descriptors/meshDescriptor/MeshDescriptor.h"
#include "../descriptorManager/modelDescriptor/descriptors/shaderProgramDescriptor/ShaderProgramDescriptor.h"
#include "../base/VulkanInitializers.hpp"
#include "../descriptorManager/buffers/LightModelPushConstant.hpp"
#include "../descriptorManager/buffers/ModelPushConstant.hpp"

namespace vks
{
    RenderPipelineManager::RenderPipelineManager(VulkanContext* context, SwapChainManager* swapChain, DescriptorManager* descriptorManager)
        : context(context), swapChain(swapChain), descriptorManager(descriptorManager)
    {
    }

    RenderPipelineManager::~RenderPipelineManager()
    {
        cleanup();
    }

    const RenderPipelineManager::Pipeline* RenderPipelineManager::findPipeline(const boost::uuids::uuid& pipelineId) const
    {
        auto it = std::find_if(pipelines.begin(), pipelines.end(),
                              [&pipelineId](const Pipeline& p) { return p.id == pipelineId; });
        return (it != pipelines.end()) ? &(*it) : nullptr;
    }

    VkPipeline RenderPipelineManager::getPipeline(const boost::uuids::uuid& pipelineId) const
    {
        const Pipeline* pipeline = findPipeline(pipelineId);
        if (!pipeline) {
            throw std::runtime_error("Pipeline not found: " + boost::uuids::to_string(pipelineId));
        }
        return pipeline->handle;
    }

    VkPipelineLayout RenderPipelineManager::getPipelineLayout(const boost::uuids::uuid& pipelineId) const
    {
        const Pipeline* pipeline = findPipeline(pipelineId);
        if (!pipeline) {
            throw std::runtime_error("Pipeline layout not found: " + boost::uuids::to_string(pipelineId));
        }
        return pipeline->layout;
    }

    VkFramebuffer RenderPipelineManager::getFramebuffer(uint32_t cameraIndex, uint32_t imageIndex) const
    {
        if (cameraIndex < cameraResources.size() && imageIndex < cameraResources[cameraIndex].framebuffers.size())
        {
            return cameraResources[cameraIndex].framebuffers[imageIndex];
        }
        return VK_NULL_HANDLE;
    }

    void RenderPipelineManager::cleanup()
    {
        for (auto& resources : cameraResources) {
            for (auto framebuffer : resources.framebuffers)
            {
                if (framebuffer != VK_NULL_HANDLE)
                    vkDestroyFramebuffer(context->getDevice(), framebuffer, nullptr);
            }
            resources.framebuffers.clear();
        }

        for (auto framebuffer : directionalShadowFramebuffers) {
            if (framebuffer != VK_NULL_HANDLE)
                vkDestroyFramebuffer(context->getDevice(), framebuffer, nullptr);
        }
        directionalShadowFramebuffers.clear();

        for (auto view : directionalShadowLayerViews) {
            if (view != VK_NULL_HANDLE)
                vkDestroyImageView(context->getDevice(), view, nullptr);
        }
        directionalShadowLayerViews.clear();

        for (auto framebuffer : pointShadowFramebuffers) {
            if (framebuffer != VK_NULL_HANDLE)
                vkDestroyFramebuffer(context->getDevice(), framebuffer, nullptr);
        }
        pointShadowFramebuffers.clear();

        for (auto view : pointShadowLayerViews) {
            if (view != VK_NULL_HANDLE)
                vkDestroyImageView(context->getDevice(), view, nullptr);
        }
        pointShadowLayerViews.clear();

        for (auto framebuffer : spotShadowFramebuffers) {
            if (framebuffer != VK_NULL_HANDLE)
                vkDestroyFramebuffer(context->getDevice(), framebuffer, nullptr);
        }
        spotShadowFramebuffers.clear();

        for (auto view : spotShadowLayerViews) {
            if (view != VK_NULL_HANDLE)
                vkDestroyImageView(context->getDevice(), view, nullptr);
        }
        spotShadowLayerViews.clear();

        if (directionalShadows.view != VK_NULL_HANDLE) {
            vkDestroyImageView(context->getDevice(), directionalShadows.view, nullptr);
            vkDestroyImage(context->getDevice(), directionalShadows.image, nullptr);
            vkFreeMemory(context->getDevice(), directionalShadows.memory, nullptr);
            directionalShadows = {};
        }

        if (pointShadows.view != VK_NULL_HANDLE) {
            vkDestroyImageView(context->getDevice(), pointShadows.view, nullptr);
            vkDestroyImage(context->getDevice(), pointShadows.image, nullptr);
            vkFreeMemory(context->getDevice(), pointShadows.memory, nullptr);
            pointShadows = {};
        }

        if (spotShadows.view != VK_NULL_HANDLE) {
            vkDestroyImageView(context->getDevice(), spotShadows.view, nullptr);
            vkDestroyImage(context->getDevice(), spotShadows.image, nullptr);
            vkFreeMemory(context->getDevice(), spotShadows.memory, nullptr);
            spotShadows = {};
        }

        cleanupDepthResources();
        cleanupOffscreenResources();

        // Cleanup all pipelines and layouts
        for (const auto& pipeline : pipelines)
        {
            if (pipeline.handle != VK_NULL_HANDLE)
            {
                vkDestroyPipeline(context->getDevice(), pipeline.handle, nullptr);
            }
            if (pipeline.layout != VK_NULL_HANDLE)
            {
                vkDestroyPipelineLayout(context->getDevice(), pipeline.layout, nullptr);
            }
        }
        pipelines.clear();

        if (renderPass != VK_NULL_HANDLE)
        {
            vkDestroyRenderPass(context->getDevice(), renderPass, nullptr);
        }

        if (shadowRenderPass != VK_NULL_HANDLE)
        {
            vkDestroyRenderPass(context->getDevice(), shadowRenderPass, nullptr);
        }

        if (shadowRenderPassMultiview != VK_NULL_HANDLE)
        {
            vkDestroyRenderPass(context->getDevice(), shadowRenderPassMultiview, nullptr);
        }

        if (pipelineCache != VK_NULL_HANDLE)
        {
            vkDestroyPipelineCache(context->getDevice(), pipelineCache, nullptr);
        }
    }

    void RenderPipelineManager::createRenderPass()
    {
        // Create attachment descriptions for color and depth
        std::array<VkAttachmentDescription, 2> attachments = {};

        // Color attachment (Offscreen scene target)
        attachments[0].format = offscreenFormat;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // Final layout for sampling in ImGui

        // Depth attachment
        attachments[1].format = VK_FORMAT_D32_SFLOAT; // Should match depth format
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


        VkAttachmentReference colorReference = {};
        colorReference.attachment = 0;
        colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthReference = {};
        depthReference.attachment = 1;
        depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpassDescription = {};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments = &colorReference;
        subpassDescription.pDepthStencilAttachment = &depthReference;
        subpassDescription.inputAttachmentCount = 0;
        subpassDescription.pInputAttachments = nullptr;
        subpassDescription.preserveAttachmentCount = 0;
        subpassDescription.pPreserveAttachments = nullptr;
        subpassDescription.pResolveAttachments = nullptr;

       // Subpass dependencies for layout transitions
			std::array<VkSubpassDependency, 2> dependencies;

			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			dependencies[1].srcSubpass = 0;
			dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        // Create render pass
        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpassDescription;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        if (vkCreateRenderPass(context->getDevice(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void RenderPipelineManager::createShadowRenderPass()
    {
        VkAttachmentDescription attachmentDescription{};
        attachmentDescription.format = shadowFormat;
        attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
        attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentReference depthReference = {};
        depthReference.attachment = 0;
        depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpassDescription = {};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = 0;
        subpassDescription.pColorAttachments = nullptr;
        subpassDescription.pDepthStencilAttachment = &depthReference;

        // Use subpass dependencies for layout transitions
        std::array<VkSubpassDependency, 2> dependencies;

        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &attachmentDescription;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpassDescription;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        if (vkCreateRenderPass(context->getDevice(), &renderPassInfo, nullptr, &shadowRenderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create shadow render pass!");
        }

        // Create multiview shadow render pass
        uint32_t viewMask = 0b00111111; // 6 bits for 6 cube faces
        uint32_t correlationMask = 0b00111111;

        VkRenderPassMultiviewCreateInfo multiviewCI{};
        multiviewCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO;
        multiviewCI.subpassCount = 1;
        multiviewCI.pViewMasks = &viewMask;
        multiviewCI.correlationMaskCount = 1;
        multiviewCI.pCorrelationMasks = &correlationMask;

        renderPassInfo.pNext = &multiviewCI;

        if (vkCreateRenderPass(context->getDevice(), &renderPassInfo, nullptr, &shadowRenderPassMultiview) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create multiview shadow render pass!");
        }
    }

    void RenderPipelineManager::createShadowResources()
    {
        // Directional Shadows (2D Array)
        {
            VkImageCreateInfo imageCI = base::initializers::imageCreateInfo();
            imageCI.imageType = VK_IMAGE_TYPE_2D;
            imageCI.extent = {SHADOWMAP_DIM, SHADOWMAP_DIM, 1};
            imageCI.mipLevels = 1;
            imageCI.arrayLayers = MAX_DIRECTIONAL_SHADOWS;
            imageCI.format = shadowFormat;
            imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateImage(context->getDevice(), &imageCI, nullptr, &directionalShadows.image) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create directional shadow image!");
            }

            VkMemoryRequirements memReqs;
            vkGetImageMemoryRequirements(context->getDevice(), directionalShadows.image, &memReqs);
            VkMemoryAllocateInfo memAlloc = base::initializers::memoryAllocateInfo();
            memAlloc.allocationSize = memReqs.size;
            memAlloc.memoryTypeIndex = context->findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            if (vkAllocateMemory(context->getDevice(), &memAlloc, nullptr, &directionalShadows.memory) != VK_SUCCESS) {
                throw std::runtime_error("Failed to allocate directional shadow memory!");
            }
            vkBindImageMemory(context->getDevice(), directionalShadows.image, directionalShadows.memory, 0);

            VkImageViewCreateInfo viewCI = base::initializers::imageViewCreateInfo();
            viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            viewCI.format = shadowFormat;
            viewCI.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, MAX_DIRECTIONAL_SHADOWS};
            viewCI.image = directionalShadows.image;
            if (vkCreateImageView(context->getDevice(), &viewCI, nullptr, &directionalShadows.view) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create directional shadow view!");
            }
        }

        // Point Shadows (Cube Array)
        {
            VkImageCreateInfo imageCI = base::initializers::imageCreateInfo();
            imageCI.imageType = VK_IMAGE_TYPE_2D;
            imageCI.extent = {SHADOWMAP_DIM, SHADOWMAP_DIM, 1};
            imageCI.mipLevels = 1;
            imageCI.arrayLayers = MAX_POINT_SHADOWS * 6;
            imageCI.format = shadowFormat;
            imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageCI.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

            if (vkCreateImage(context->getDevice(), &imageCI, nullptr, &pointShadows.image) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create point shadow image!");
            }

            VkMemoryRequirements memReqs;
            vkGetImageMemoryRequirements(context->getDevice(), pointShadows.image, &memReqs);
            VkMemoryAllocateInfo memAlloc = base::initializers::memoryAllocateInfo();
            memAlloc.allocationSize = memReqs.size;
            memAlloc.memoryTypeIndex = context->findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            if (vkAllocateMemory(context->getDevice(), &memAlloc, nullptr, &pointShadows.memory) != VK_SUCCESS) {
                throw std::runtime_error("Failed to allocate point shadow memory!");
            }
            vkBindImageMemory(context->getDevice(), pointShadows.image, pointShadows.memory, 0);

            VkImageViewCreateInfo viewCI = base::initializers::imageViewCreateInfo();
            viewCI.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
            viewCI.format = shadowFormat;
            viewCI.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, MAX_POINT_SHADOWS * 6};
            viewCI.image = pointShadows.image;
            if (vkCreateImageView(context->getDevice(), &viewCI, nullptr, &pointShadows.view) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create point shadow view!");
            }
        }

        // Spot Shadows (2D Array)
        {
            VkImageCreateInfo imageCI = base::initializers::imageCreateInfo();
            imageCI.imageType = VK_IMAGE_TYPE_2D;
            imageCI.extent = {SHADOWMAP_DIM, SHADOWMAP_DIM, 1};
            imageCI.mipLevels = 1;
            imageCI.arrayLayers = MAX_SPOT_SHADOWS;
            imageCI.format = shadowFormat;
            imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateImage(context->getDevice(), &imageCI, nullptr, &spotShadows.image) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create spot shadow image!");
            }

            VkMemoryRequirements memReqs;
            vkGetImageMemoryRequirements(context->getDevice(), spotShadows.image, &memReqs);
            VkMemoryAllocateInfo memAlloc = base::initializers::memoryAllocateInfo();
            memAlloc.allocationSize = memReqs.size;
            memAlloc.memoryTypeIndex = context->findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            if (vkAllocateMemory(context->getDevice(), &memAlloc, nullptr, &spotShadows.memory) != VK_SUCCESS) {
                throw std::runtime_error("Failed to allocate spot shadow memory!");
            }
            vkBindImageMemory(context->getDevice(), spotShadows.image, spotShadows.memory, 0);

            VkImageViewCreateInfo viewCI = base::initializers::imageViewCreateInfo();
            viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            viewCI.format = shadowFormat;
            viewCI.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, MAX_SPOT_SHADOWS};
            viewCI.image = spotShadows.image;
            if (vkCreateImageView(context->getDevice(), &viewCI, nullptr, &spotShadows.view) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create spot shadow view!");
            }
        }

        // Ensure all shadow images have a valid layout and are cleared before first use via descriptors
        {
            VkCommandBuffer cmdBuffer = context->beginSingleTimeCommands(QueueType::Graphics);

            VkImageMemoryBarrier barriers[3]{};

            // Transition to TRANSFER_DST_OPTIMAL to clear
            for (int i = 0; i < 3; i++) {
                barriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barriers[i].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                barriers[i].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barriers[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                barriers[i].subresourceRange.baseMipLevel = 0;
                barriers[i].subresourceRange.levelCount = 1;
                barriers[i].subresourceRange.baseArrayLayer = 0;
                barriers[i].srcAccessMask = 0;
                barriers[i].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            }

            barriers[0].image = directionalShadows.image;
            barriers[0].subresourceRange.layerCount = MAX_DIRECTIONAL_SHADOWS;
            barriers[1].image = pointShadows.image;
            barriers[1].subresourceRange.layerCount = MAX_POINT_SHADOWS * 6;
            barriers[2].image = spotShadows.image;
            barriers[2].subresourceRange.layerCount = MAX_SPOT_SHADOWS;

            vkCmdPipelineBarrier(
                cmdBuffer,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                3, barriers);

            VkClearDepthStencilValue clearValue = {1.0f, 0};

            vkCmdClearDepthStencilImage(cmdBuffer, directionalShadows.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearValue, 1, &barriers[0].subresourceRange);
            vkCmdClearDepthStencilImage(cmdBuffer, pointShadows.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearValue, 1, &barriers[1].subresourceRange);
            vkCmdClearDepthStencilImage(cmdBuffer, spotShadows.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearValue, 1, &barriers[2].subresourceRange);

            // Transition to SHADER_READ_ONLY_OPTIMAL
            for (int i = 0; i < 3; i++) {
                barriers[i].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barriers[i].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barriers[i].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            }

            vkCmdPipelineBarrier(
                cmdBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                3, barriers);

            context->endSingleTimeCommands(cmdBuffer, QueueType::Graphics);
        }
    }

    void RenderPipelineManager::createShadowFramebuffers()
    {
        // Directional Shadow Framebuffers
        directionalShadowFramebuffers.resize(MAX_DIRECTIONAL_SHADOWS);
        directionalShadowLayerViews.resize(MAX_DIRECTIONAL_SHADOWS);
        for (uint32_t i = 0; i < MAX_DIRECTIONAL_SHADOWS; i++) {
            VkImageViewCreateInfo viewCI = base::initializers::imageViewCreateInfo();
            viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewCI.format = shadowFormat;
            viewCI.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, i, 1};
            viewCI.image = directionalShadows.image;
            if (vkCreateImageView(context->getDevice(), &viewCI, nullptr, &directionalShadowLayerViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create directional shadow layer view!");
            }

            VkFramebufferCreateInfo framebufferCI = base::initializers::framebufferCreateInfo();
            framebufferCI.renderPass = shadowRenderPass;
            framebufferCI.attachmentCount = 1;
            framebufferCI.pAttachments = &directionalShadowLayerViews[i];
            framebufferCI.width = SHADOWMAP_DIM;
            framebufferCI.height = SHADOWMAP_DIM;
            framebufferCI.layers = 1;

            if (vkCreateFramebuffer(context->getDevice(), &framebufferCI, nullptr, &directionalShadowFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create directional shadow framebuffer!");
            }
        }

        // Point Shadow Framebuffers
        pointShadowFramebuffers.resize(MAX_POINT_SHADOWS);
        pointShadowLayerViews.resize(MAX_POINT_SHADOWS);
        for (uint32_t i = 0; i < MAX_POINT_SHADOWS; i++) {
            VkImageViewCreateInfo viewCI = base::initializers::imageViewCreateInfo();
            viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            viewCI.format = shadowFormat;
            viewCI.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, i * 6, 6};
            viewCI.image = pointShadows.image;
            if (vkCreateImageView(context->getDevice(), &viewCI, nullptr, &pointShadowLayerViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create point shadow layer view!");
            }

            VkFramebufferCreateInfo framebufferCI = base::initializers::framebufferCreateInfo();
            framebufferCI.renderPass = shadowRenderPassMultiview;
            framebufferCI.attachmentCount = 1;
            framebufferCI.pAttachments = &pointShadowLayerViews[i];
            framebufferCI.width = SHADOWMAP_DIM;
            framebufferCI.height = SHADOWMAP_DIM;
            framebufferCI.layers = 1;

            if (vkCreateFramebuffer(context->getDevice(), &framebufferCI, nullptr, &pointShadowFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create point shadow framebuffer!");
            }
        }

        // Spot Shadow Framebuffers
        spotShadowFramebuffers.resize(MAX_SPOT_SHADOWS);
        spotShadowLayerViews.resize(MAX_SPOT_SHADOWS);
        for (uint32_t i = 0; i < MAX_SPOT_SHADOWS; i++) {
            VkImageViewCreateInfo viewCI = base::initializers::imageViewCreateInfo();
            viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewCI.format = shadowFormat;
            viewCI.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, i, 1};
            viewCI.image = spotShadows.image;
            if (vkCreateImageView(context->getDevice(), &viewCI, nullptr, &spotShadowLayerViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create spot shadow layer view!");
            }

            VkFramebufferCreateInfo framebufferCI = base::initializers::framebufferCreateInfo();
            framebufferCI.renderPass = shadowRenderPass;
            framebufferCI.attachmentCount = 1;
            framebufferCI.pAttachments = &spotShadowLayerViews[i];
            framebufferCI.width = SHADOWMAP_DIM;
            framebufferCI.height = SHADOWMAP_DIM;
            framebufferCI.layers = 1;

            if (vkCreateFramebuffer(context->getDevice(), &framebufferCI, nullptr, &spotShadowFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create spot shadow framebuffer!");
            }
        }
    }


    void RenderPipelineManager::createFramebuffers(VkExtent2D swapChainExtent)
    {
        uint32_t maxCameras = 4;
        if (cameraResources.size() < maxCameras) {
            cameraResources.resize(maxCameras);
        }

        for (uint32_t camIdx = 0; camIdx < maxCameras; camIdx++) {
            auto& resources = cameraResources[camIdx];

            // Clean up old framebuffers if they exist
            for (auto framebuffer : resources.framebuffers)
            {
                if (framebuffer != VK_NULL_HANDLE)
                    vkDestroyFramebuffer(context->getDevice(), framebuffer, nullptr);
            }

            // Resize framebuffers vector to match number of offscreen targets (usually matches swapchain)
            resources.framebuffers.resize(resources.offscreenTargets.size());

            // Create a framebuffer for each offscreen target
            for (size_t i = 0; i < resources.offscreenTargets.size(); i++)
            {
                std::array<VkImageView, 2> attachments = {
                    resources.offscreenTargets[i].view, // Color attachment (offscreen)
                    resources.depthImageView // Depth attachment
                };

                VkFramebufferCreateInfo framebufferInfo{};
                framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferInfo.renderPass = renderPass;
                framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
                framebufferInfo.pAttachments = attachments.data();
                framebufferInfo.width = swapChainExtent.width;
                framebufferInfo.height = swapChainExtent.height;
                framebufferInfo.layers = 1;

                if (vkCreateFramebuffer(context->getDevice(), &framebufferInfo, nullptr, &resources.framebuffers[i]) != VK_SUCCESS)
                {
                    throw std::runtime_error("Failed to create offscreen framebuffer!");
                }
            }
        }
    }

    void RenderPipelineManager::createOffscreenResources(VkExtent2D extent)
    {
        cleanupOffscreenResources();

        uint32_t maxCameras = 4;
        cameraResources.resize(maxCameras);

        uint32_t imageCount = static_cast<uint32_t>(swapChain->getImageViews().size());

        for (uint32_t camIdx = 0; camIdx < maxCameras; camIdx++) {
            auto& resources = cameraResources[camIdx];
            resources.offscreenTargets.resize(imageCount);

            for (uint32_t i = 0; i < imageCount; i++) {
                VkImageCreateInfo imageInfo{};
                imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                imageInfo.imageType = VK_IMAGE_TYPE_2D;
                imageInfo.extent.width = extent.width;
                imageInfo.extent.height = extent.height;
                imageInfo.extent.depth = 1;
                imageInfo.mipLevels = 1;
                imageInfo.arrayLayers = 1;
                imageInfo.format = offscreenFormat;
                imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
                imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                if (vkCreateImage(context->getDevice(), &imageInfo, nullptr, &resources.offscreenTargets[i].image) != VK_SUCCESS) {
                    throw std::runtime_error("failed to create offscreen image!");
                }

                VkMemoryRequirements memRequirements;
                vkGetImageMemoryRequirements(context->getDevice(), resources.offscreenTargets[i].image, &memRequirements);

                VkMemoryAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                allocInfo.allocationSize = memRequirements.size;
                allocInfo.memoryTypeIndex = context->findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

                if (vkAllocateMemory(context->getDevice(), &allocInfo, nullptr, &resources.offscreenTargets[i].memory) != VK_SUCCESS) {
                    throw std::runtime_error("failed to allocate offscreen image memory!");
                }

                vkBindImageMemory(context->getDevice(), resources.offscreenTargets[i].image, resources.offscreenTargets[i].memory, 0);

                VkImageViewCreateInfo viewInfo{};
                viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                viewInfo.image = resources.offscreenTargets[i].image;
                viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                viewInfo.format = offscreenFormat;
                viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                viewInfo.subresourceRange.baseMipLevel = 0;
                viewInfo.subresourceRange.levelCount = 1;
                viewInfo.subresourceRange.baseArrayLayer = 0;
                viewInfo.subresourceRange.layerCount = 1;

                if (vkCreateImageView(context->getDevice(), &viewInfo, nullptr, &resources.offscreenTargets[i].view) != VK_SUCCESS) {
                    throw std::runtime_error("failed to create offscreen image view!");
                }

                // Explicitly transition to SHADER_READ_ONLY_OPTIMAL for the first use
                VkCommandBuffer cmdBuffer = context->beginSingleTimeCommands(QueueType::Graphics);
                VkImageMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = resources.offscreenTargets[i].image;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                vkCmdPipelineBarrier(
                    cmdBuffer,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier);

                context->endSingleTimeCommands(cmdBuffer, QueueType::Graphics);
            }
        }
    }

    void RenderPipelineManager::cleanupOffscreenResources()
    {
        for (auto& resources : cameraResources) {
            for (auto& target : resources.offscreenTargets) {
                if (target.view != VK_NULL_HANDLE) vkDestroyImageView(context->getDevice(), target.view, nullptr);
                if (target.image != VK_NULL_HANDLE) vkDestroyImage(context->getDevice(), target.image, nullptr);
                if (target.memory != VK_NULL_HANDLE) vkFreeMemory(context->getDevice(), target.memory, nullptr);
            }
            resources.offscreenTargets.clear();
        }
    }

    void RenderPipelineManager::createShadowPipeline(ShaderProgramDescriptor* shaderProgramDescriptor)
    {
        boost::uuids::uuid pipelineId = shaderProgramDescriptor->getAssetId();
        createPipelineCache();

        if (findPipeline(pipelineId)) {
            throw std::runtime_error("Pipeline already exists: " + boost::uuids::to_string(pipelineId));
        }

        const auto& combinedDefines = shaderProgramDescriptor->getDefines();

        // Get layouts for each define - ensuring they are at the correct set index
        // We know the set indices: 0: Scene, 1: Material, 2: Mesh, 3: Lights
        std::vector<VkDescriptorSetLayout> combinedLayouts;

        // Find max define index to determine layout count
        int maxSet = 0;
        for (auto def : combinedDefines) {
            if (def == ShaderDefinesEnum::SCENE_UBO_GLSL) maxSet = std::max(maxSet, 0);
            else if (def == ShaderDefinesEnum::MATERIAL_PBR_GLSL) maxSet = std::max(maxSet, 1);
            else if (def == ShaderDefinesEnum::MATERIAL_SKYBOX_GLSL) maxSet = std::max(maxSet, 1);
            else if (def == ShaderDefinesEnum::VERTEX_IO_GLSL) maxSet = std::max(maxSet, 2);
            else if (def == ShaderDefinesEnum::LIGHTING_COMMON_GLSL) maxSet = std::max(maxSet, 3);
            else if (def == ShaderDefinesEnum::LIGHT_MODEL_PC_GLSL) maxSet = std::max(maxSet, 3);
        }

        combinedLayouts.resize(maxSet + 1, VK_NULL_HANDLE);

        // Fill them based on define
        for (auto def : combinedDefines) {
            switch (def) {
            case ShaderDefinesEnum::SCENE_UBO_GLSL: combinedLayouts[0] = descriptorManager->getSceneLayout(); break;
            case ShaderDefinesEnum::MATERIAL_PBR_GLSL: combinedLayouts[1] = descriptorManager->getPbrMaterialLayout(); break;
            case ShaderDefinesEnum::MATERIAL_SKYBOX_GLSL: combinedLayouts[1] = descriptorManager->getSkyboxMaterialLayout(); break;
            case ShaderDefinesEnum::VERTEX_IO_GLSL: combinedLayouts[2] = descriptorManager->getMeshUniformLayout(); break;
            case ShaderDefinesEnum::LIGHTING_COMMON_GLSL: combinedLayouts[3] = descriptorManager->getLightsLayout(); break;
            case ShaderDefinesEnum::LIGHT_MODEL_PC_GLSL: combinedLayouts[3] = descriptorManager->getLightsLayout(); break;
            }
        }

        // Check for any null layouts in the sequence (up to maxSet)
        // If a shader uses Set N, it must have valid layouts for 0..N-1
        for (int i = 0; i <= maxSet; ++i) {
            if (combinedLayouts[i] == VK_NULL_HANDLE) {
                // Use scene layout as a placeholder for missing sets
                combinedLayouts[i] = descriptorManager->getSceneLayout();
            }
        }

        VkPipelineLayout shadowPipelineLayout = VK_NULL_HANDLE;
        VkPipelineLayoutCreateInfo shadowPipelineLayoutInfo{};
        shadowPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        shadowPipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(combinedLayouts.size());
        shadowPipelineLayoutInfo.pSetLayouts = combinedLayouts.data();

        std::vector<VkPushConstantRange> pushConstantRanges;
        
        if (std::find(combinedDefines.begin(), combinedDefines.end(), ShaderDefinesEnum::MODEL_PC_GLSL) != combinedDefines.end())
        {
            VkPushConstantRange modelPCRange{};
            modelPCRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            modelPCRange.offset = 0;
            modelPCRange.size = sizeof(ModelPushConstant);
            pushConstantRanges.push_back(modelPCRange);
        }
        
        if (std::find(combinedDefines.begin(), combinedDefines.end(), ShaderDefinesEnum::LIGHT_MODEL_PC_GLSL) != combinedDefines.end())
        {
            VkPushConstantRange lightModelPCRange{};
            lightModelPCRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            lightModelPCRange.offset = 0;
            lightModelPCRange.size = sizeof(LightModelPushConstant);
            pushConstantRanges.push_back(lightModelPCRange);
        }

        shadowPipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
        shadowPipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

        if (vkCreatePipelineLayout(context->getDevice(), &shadowPipelineLayoutInfo, nullptr, &shadowPipelineLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create shadow pipeline layout!");
        }

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = base::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
        VkPipelineViewportStateCreateInfo viewportState = base::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
        VkPipelineMultisampleStateCreateInfo multisampleState = base::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
        std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS };
        VkPipelineDynamicStateCreateInfo dynamicState = base::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
        
        VkPipelineColorBlendAttachmentState blendAttachmentState = base::initializers::pipelineColorBlendAttachmentState(0, VK_FALSE);
        VkPipelineColorBlendStateCreateInfo colorBlendState = base::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

        const auto& shaderStages = shaderProgramDescriptor->getShaderStages();

        VkPipelineRasterizationStateCreateInfo rasterizationState = base::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
        rasterizationState.depthBiasEnable = VK_TRUE;

        VkPipelineDepthStencilStateCreateInfo depthStencilState = base::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);

        VkGraphicsPipelineCreateInfo pipelineCI = vks::base::initializers::pipelineCreateInfo(shadowPipelineLayout, shadowRenderPass, 0);
        
        // Use multiview render pass for point shadow shaders (which use multiple views)
        bool isMultiview = std::find(combinedDefines.begin(), combinedDefines.end(), ShaderDefinesEnum::LIGHTING_COMMON_GLSL) != combinedDefines.end() ||
                           std::find(combinedDefines.begin(), combinedDefines.end(), ShaderDefinesEnum::LIGHT_MODEL_PC_GLSL) != combinedDefines.end();
        
        if (isMultiview) {
            pipelineCI.renderPass = shadowRenderPassMultiview;
        }

        pipelineCI.pInputAssemblyState = &inputAssemblyState;
        pipelineCI.pViewportState = &viewportState;
        pipelineCI.pRasterizationState = &rasterizationState;
        pipelineCI.pMultisampleState = &multisampleState;
        pipelineCI.pDepthStencilState = &depthStencilState;
        pipelineCI.pColorBlendState = &colorBlendState;
        pipelineCI.pDynamicState = &dynamicState;
        pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineCI.pStages = shaderStages.data();

        pipelineCI.pVertexInputState = MeshDescriptor::getPipelineVertexInputState({ VertexComponent::Position });

        VkPipeline pipelineHandle = VK_NULL_HANDLE;
        if (vkCreateGraphicsPipelines(context->getDevice(), pipelineCache, 1, &pipelineCI, nullptr, &pipelineHandle) != VK_SUCCESS)
        {
            vkDestroyPipelineLayout(context->getDevice(), shadowPipelineLayout, nullptr);
            throw std::runtime_error("failed to create shadow graphics pipeline!");
        }

        pipelines.push_back(Pipeline{pipelineId, pipelineHandle, shadowPipelineLayout});
    }

     void RenderPipelineManager::createGraphicsPipeline(ShaderProgramDescriptor* shaderProgramDescriptor)
    {
        boost::uuids::uuid pipelineId = shaderProgramDescriptor->getAssetId();
        createPipelineCache();

        // Check if pipeline already exists
        if (findPipeline(pipelineId)) {
            throw std::runtime_error("Pipeline already exists: " + boost::uuids::to_string(pipelineId));
        }

        const auto& combinedDefines = shaderProgramDescriptor->getDefines();

        // Get layouts for each define - ensuring they are at the correct set index
        // We know the set indices: 0: Scene, 1: Material, 2: Mesh, 3: Lights
        // DescriptorManager::getLayoutsFromEnums should ideally return them in the correct order,
        // but it doesn't guarantee the size or empty slots.

        // Let's get them one by one or filter.
        std::vector<VkDescriptorSetLayout> combinedLayouts;

        // Find max define index to determine layout count
        int maxSet = 0;
        for (auto def : combinedDefines) {
            if (def == ShaderDefinesEnum::SCENE_UBO_GLSL) maxSet = std::max(maxSet, 0);
            else if (def == ShaderDefinesEnum::MATERIAL_PBR_GLSL) maxSet = std::max(maxSet, 1);
            else if (def == ShaderDefinesEnum::MATERIAL_SKYBOX_GLSL) maxSet = std::max(maxSet, 1);
            else if (def == ShaderDefinesEnum::VERTEX_IO_GLSL) maxSet = std::max(maxSet, 2);
            else if (def == ShaderDefinesEnum::LIGHTING_COMMON_GLSL) maxSet = std::max(maxSet, 3);
            else if (def == ShaderDefinesEnum::LIGHT_MODEL_PC_GLSL) maxSet = std::max(maxSet, 3);
        }

        combinedLayouts.resize(maxSet + 1, VK_NULL_HANDLE);

        // Fill them based on define
        for (auto def : combinedDefines) {
            switch (def) {
            case ShaderDefinesEnum::SCENE_UBO_GLSL: combinedLayouts[0] = descriptorManager->getSceneLayout(); break;
            case ShaderDefinesEnum::MATERIAL_PBR_GLSL: combinedLayouts[1] = descriptorManager->getPbrMaterialLayout(); break;
            case ShaderDefinesEnum::MATERIAL_SKYBOX_GLSL: combinedLayouts[1] = descriptorManager->getSkyboxMaterialLayout(); break;
            case ShaderDefinesEnum::VERTEX_IO_GLSL: combinedLayouts[2] = descriptorManager->getMeshUniformLayout(); break;
            case ShaderDefinesEnum::LIGHTING_COMMON_GLSL: combinedLayouts[3] = descriptorManager->getLightsLayout(); break;
            case ShaderDefinesEnum::LIGHT_MODEL_PC_GLSL: combinedLayouts[3] = descriptorManager->getLightsLayout(); break;
            }
        }

        // Check for any null layouts in the sequence (up to maxSet)
        for (int i = 0; i <= maxSet; ++i) {
            if (combinedLayouts[i] == VK_NULL_HANDLE) {
                combinedLayouts[i] = descriptorManager->getSceneLayout();
            }
        }

        VkPipelineLayout meshPipelineLayout = VK_NULL_HANDLE;
        VkPipelineLayoutCreateInfo meshPipelineLayoutInfo{};
        meshPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        meshPipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(combinedLayouts.size());
        meshPipelineLayoutInfo.pSetLayouts = combinedLayouts.data();

        VkPushConstantRange modelPCRange{};
        VkPushConstantRange lightModelPCRange{};
        std::vector<VkPushConstantRange> pushConstantRanges;

        if (std::find(combinedDefines.begin(), combinedDefines.end(), ShaderDefinesEnum::MODEL_PC_GLSL) != combinedDefines.end())
        {
            // Define push constant range for the vertex shader transform matrix
            modelPCRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            modelPCRange.offset = 0;
            modelPCRange.size = sizeof(ModelPushConstant);
            pushConstantRanges.push_back(modelPCRange);
        }
        
        if (std::find(combinedDefines.begin(), combinedDefines.end(), ShaderDefinesEnum::LIGHT_MODEL_PC_GLSL) != combinedDefines.end())
        {
            // Define push constant range for the light shadow model push constant
            lightModelPCRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            lightModelPCRange.offset = 0;
            lightModelPCRange.size = sizeof(LightModelPushConstant);
            pushConstantRanges.push_back(lightModelPCRange);
        }

        if (!pushConstantRanges.empty()) {
            meshPipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
            meshPipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();
        }

        if (vkCreatePipelineLayout(context->getDevice(), &meshPipelineLayoutInfo, nullptr, &meshPipelineLayout) !=
             VK_SUCCESS)
        {
            throw std::runtime_error("failed to create mesh pipeline layout!");
        }

        // Common pipeline settings
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
            base::initializers::pipelineInputAssemblyStateCreateInfo(
                VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

        VkPipelineViewportStateCreateInfo viewportState =
            base::initializers::pipelineViewportStateCreateInfo(1, 1, 0);

        VkPipelineMultisampleStateCreateInfo multisampleState =
            base::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);

        std::vector<VkDynamicState> dynamicStateEnables = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState =
            base::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);

        // Shared blend attachment state
        VkPipelineColorBlendAttachmentState blendAttachmentState =
            base::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);

        VkPipelineColorBlendStateCreateInfo colorBlendState =
            base::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

        // Load shaders
        const auto& shaderStages = shaderProgramDescriptor->getShaderStages();

        // Mesh-specific rasterization state
        VkPipelineRasterizationStateCreateInfo rasterizationState =
            base::initializers::pipelineRasterizationStateCreateInfo(
                VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);

        // Mesh-specific depth state
        VkPipelineDepthStencilStateCreateInfo depthStencilState =
            base::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);

        if (std::find(combinedDefines.begin(), combinedDefines.end(), ShaderDefinesEnum::MATERIAL_SKYBOX_GLSL) != combinedDefines.end())
        {
            rasterizationState.cullMode = VK_CULL_MODE_NONE;
        }

        // Create mesh pipeline
        VkGraphicsPipelineCreateInfo pipelineCI = vks::base::initializers::pipelineCreateInfo(
            meshPipelineLayout, renderPass, 0);
        pipelineCI.pInputAssemblyState = &inputAssemblyState;
        pipelineCI.pViewportState = &viewportState;
        pipelineCI.pRasterizationState = &rasterizationState;
        pipelineCI.pMultisampleState = &multisampleState;
        pipelineCI.pDepthStencilState = &depthStencilState;
        pipelineCI.pColorBlendState = &colorBlendState;
        pipelineCI.pDynamicState = &dynamicState;
        pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineCI.pStages = shaderStages.data();

        if (std::find(combinedDefines.begin(), combinedDefines.end(), ShaderDefinesEnum::MODEL_PC_GLSL) != combinedDefines.end() ||
            std::find(combinedDefines.begin(), combinedDefines.end(), ShaderDefinesEnum::LIGHT_MODEL_PC_GLSL) != combinedDefines.end())
        {
            pipelineCI.pVertexInputState = MeshDescriptor::getPipelineVertexInputState({
                VertexComponent::Position,
                VertexComponent::Normal,
                VertexComponent::UV,
                VertexComponent::Color,
                VertexComponent::Tangent,
                VertexComponent::Bitangent
            });
        }
        VkPipeline pipelineHandle = VK_NULL_HANDLE;
        if (vkCreateGraphicsPipelines(context->getDevice(), pipelineCache, 1, &pipelineCI, nullptr, &pipelineHandle)
            != VK_SUCCESS)
        {
            vkDestroyPipelineLayout(context->getDevice(), meshPipelineLayout, nullptr);
            throw std::runtime_error("failed to create mesh graphics pipeline!");
        }

        // Add pipeline to vector
        pipelines.push_back(Pipeline{pipelineId, pipelineHandle, meshPipelineLayout});
    }

    void RenderPipelineManager::createPipelineCache()
    {
        if (pipelineCache != VK_NULL_HANDLE) {
            return; // Cache already created
        }

        VkPipelineCacheCreateInfo pipelineCacheCreateInfo{};
        pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

        if (vkCreatePipelineCache(context->getDevice(), &pipelineCacheCreateInfo, nullptr, &pipelineCache) !=
            VK_SUCCESS)
        {
            throw std::runtime_error("failed to create pipeline cache!");
        }
    }

    void RenderPipelineManager::createDepthResources(VkExtent2D swapChainExtent)
    {
        VkFormat depthFormat = VK_FORMAT_D32_SFLOAT; // Should match the format in createRenderPass

        uint32_t maxCameras = 4;
        if (cameraResources.size() < maxCameras) {
            cameraResources.resize(maxCameras);
        }

        for (uint32_t camIdx = 0; camIdx < maxCameras; camIdx++) {
            auto& resources = cameraResources[camIdx];

            // Create depth image
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = swapChainExtent.width;
            imageInfo.extent.height = swapChainExtent.height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = depthFormat;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateImage(context->getDevice(), &imageInfo, nullptr, &resources.depthImage) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create depth image!");
            }

            // Get memory requirements and allocate
            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(context->getDevice(), resources.depthImage, &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = context->findMemoryType(memRequirements.memoryTypeBits,
                                                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            if (vkAllocateMemory(context->getDevice(), &allocInfo, nullptr, &resources.depthImageMemory) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to allocate depth image memory!");
            }

            vkBindImageMemory(context->getDevice(), resources.depthImage, resources.depthImageMemory, 0);

            // Create image view
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = resources.depthImage;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = depthFormat;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(context->getDevice(), &viewInfo, nullptr, &resources.depthImageView) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create depth image view!");
            }

            // Explicitly transition to DEPTH_STENCIL_ATTACHMENT_OPTIMAL for the first use
            VkCommandBuffer cmdBuffer = context->beginSingleTimeCommands(QueueType::Graphics);
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = resources.depthImage;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            vkCmdPipelineBarrier(
                cmdBuffer,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            context->endSingleTimeCommands(cmdBuffer, QueueType::Graphics);
        }
    }

    void RenderPipelineManager::cleanupDepthResources()
    {
        for (auto& resources : cameraResources) {
            if (resources.depthImageView != VK_NULL_HANDLE)
            {
                vkDestroyImageView(context->getDevice(), resources.depthImageView, nullptr);
                resources.depthImageView = VK_NULL_HANDLE;
            }
            if (resources.depthImage != VK_NULL_HANDLE)
            {
                vkDestroyImage(context->getDevice(), resources.depthImage, nullptr);
                resources.depthImage = VK_NULL_HANDLE;
            }
            if (resources.depthImageMemory != VK_NULL_HANDLE)
            {
                vkFreeMemory(context->getDevice(), resources.depthImageMemory, nullptr);
                resources.depthImageMemory = VK_NULL_HANDLE;
            }
        }
    }
} // namespace vks