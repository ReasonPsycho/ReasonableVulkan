#include <boost/uuid/uuid_io.hpp>
#include "RenderPipelineManager.hpp"
#include <stdexcept>
#include <algorithm>
#include "../descriptorManager/modelDescriptor/descriptors/meshDescriptor/MeshDescriptor.h"
#include "../descriptorManager/modelDescriptor/descriptors/shaderProgramDescriptor/ShaderProgramDescriptor.h"
#include "../base/VulkanInitializers.hpp"
#include "../base/VulkanTools.h"

namespace vks
{
    RenderPipelineManager::RenderPipelineManager(VulkanContext* context, DescriptorManager* descriptorManager)
        : context(context), descriptorManager(descriptorManager)
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

    void RenderPipelineManager::cleanup()
    {
        for (auto framebuffer : framebuffers)
        {
            vkDestroyFramebuffer(context->getDevice(), framebuffer, nullptr);
        }

        cleanupDepthResources();

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

        if (pipelineCache != VK_NULL_HANDLE)
        {
            vkDestroyPipelineCache(context->getDevice(), pipelineCache, nullptr);
        }
    }

    void RenderPipelineManager::createRenderPass()
    {
        // Create attachment descriptions for color and depth
        std::array<VkAttachmentDescription, 2> attachments = {};

        // Color attachment
        attachments[0].format = VK_FORMAT_B8G8R8A8_SRGB; // Should match swapchain format
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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


    void RenderPipelineManager::createFramebuffers(const std::vector<VkImageView>& swapChainImageViews,
                                                   VkExtent2D swapChainExtent)
    {
        // Clean up old framebuffers if they exist
        for (auto framebuffer : framebuffers)
        {
            vkDestroyFramebuffer(context->getDevice(), framebuffer, nullptr);
        }

        // Resize framebuffers vector to match number of swap chain images
        framebuffers.resize(swapChainImageViews.size());

        // Create a framebuffer for each swap chain image view
        for (size_t i = 0; i < swapChainImageViews.size(); i++)
        {
            std::array<VkImageView, 2> attachments = {
                swapChainImageViews[i], // Color attachment
                depthImageView // Depth attachment
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(context->getDevice(), &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create framebuffer!");
            }
        }
    }

    void RenderPipelineManager::createGraphicsPipeline(ShaderProgramDescriptor* shaderProgramDescriptor)
    {
        boost::uuids::uuid pipelineId = shaderProgramDescriptor->getAssetId();
        createPipelineCache();

        // Check if pipeline already exists
        if (findPipeline(pipelineId)) {
            throw std::runtime_error("Pipeline already exists: " + boost::uuids::to_string(pipelineId));
        }

        const auto& combinedDefines = shaderProgramDescriptor->getCombinedDefines();

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
            }
        }

        // Check for any null layouts in the sequence (up to maxSet)
        for (int i = 0; i <= maxSet; ++i) {
            if (combinedLayouts[i] == VK_NULL_HANDLE) {
                // If a shader skips a set, we still need a valid (but possibly empty) layout if it's not the last one
                // Use the lights layout as a fallback if it's empty, or better, the mesh layout if it's just a UBO.
                // Actually, DescriptorManager should provide an empty layout.
                // For now, let's use scene layout as it's a simple UBO layout, 
                // but ideally we should have a truly empty layout.
                // However, most of our layouts are not empty.
                // Let's see if we have any other option. 
                // Actually, let's use the scene layout as a placeholder if it exists.
                combinedLayouts[i] = descriptorManager->getSceneLayout();
            }
        }

        VkPipelineLayout meshPipelineLayout = VK_NULL_HANDLE;
        VkPipelineLayoutCreateInfo meshPipelineLayoutInfo{};
        meshPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        meshPipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(combinedLayouts.size());
        meshPipelineLayoutInfo.pSetLayouts = combinedLayouts.data();

        VkPushConstantRange pushConstantRange{};
        if (std::find(combinedDefines.begin(), combinedDefines.end(), ShaderDefinesEnum::MODEL_PC_GLSL) != combinedDefines.end())
        {
            meshPipelineLayoutInfo.pushConstantRangeCount = 1;

            // Define push constant range for the vertex shader transform matrix
            pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            pushConstantRange.offset = 0;
            pushConstantRange.size = sizeof(glm::mat4);

            meshPipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
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

        if (std::find(combinedDefines.begin(), combinedDefines.end(), ShaderDefinesEnum::MODEL_PC_GLSL) != combinedDefines.end())
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
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(context->getDevice(), &imageInfo, nullptr, &depthImage) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create depth image!");
        }

        // Get memory requirements and allocate
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(context->getDevice(), depthImage, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = context->findMemoryType(memRequirements.memoryTypeBits,
                                                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(context->getDevice(), &allocInfo, nullptr, &depthImageMemory) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate depth image memory!");
        }

        vkBindImageMemory(context->getDevice(), depthImage, depthImageMemory, 0);

        // Create image view
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = depthImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = depthFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(context->getDevice(), &viewInfo, nullptr, &depthImageView) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create depth image view!");
        }
    }

    void RenderPipelineManager::cleanupDepthResources()
    {
        if (depthImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(context->getDevice(), depthImageView, nullptr);
            depthImageView = VK_NULL_HANDLE;
        }
        if (depthImage != VK_NULL_HANDLE)
        {
            vkDestroyImage(context->getDevice(), depthImage, nullptr);
            depthImage = VK_NULL_HANDLE;
        }
        if (depthImageMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(context->getDevice(), depthImageMemory, nullptr);
            depthImageMemory = VK_NULL_HANDLE;
        }
    }
} // namespace vks