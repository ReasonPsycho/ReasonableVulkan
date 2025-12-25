
#include "RenderPipelineManager.hpp"
#include <stdexcept>

#include "../descriptorManager/modelDescriptor/descriptors/meshDescriptor/MeshDescriptor.h"

namespace vks
{
    RenderPipelineManager::RenderPipelineManager(VulkanContext* context, DescriptorManager* descriptorManager)
        : context(context), descriptorManager(descriptorManager) {
    }

    RenderPipelineManager::~RenderPipelineManager() {
        cleanup();
    }

    void RenderPipelineManager::cleanup() {
        for (auto framebuffer : framebuffers) {
            vkDestroyFramebuffer(context->getDevice(), framebuffer, nullptr);
        }

        cleanupDepthResources();

        if (pipelines.models != VK_NULL_HANDLE) {
            vkDestroyPipeline(context->getDevice(), pipelines.models, nullptr);
        }
        if (pipelines.skybox != VK_NULL_HANDLE) {
            vkDestroyPipeline(context->getDevice(), pipelines.skybox, nullptr);
        }

        if (meshPipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(context->getDevice(), meshPipelineLayout, nullptr);
            meshPipelineLayout = VK_NULL_HANDLE;
        }

        if (skyboxPipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(context->getDevice(), skyboxPipelineLayout, nullptr);
            skyboxPipelineLayout = VK_NULL_HANDLE;
        }

        if (renderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(context->getDevice(), renderPass, nullptr);
        }

        if (pipelineCache != VK_NULL_HANDLE) {
            vkDestroyPipelineCache(context->getDevice(), pipelineCache, nullptr);
        }
    }

    void RenderPipelineManager::createRenderPass() {
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
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // Depth attachment
        attachments[1].format = VK_FORMAT_D32_SFLOAT; // Should match depth format
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // Create subpass
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        // Create render pass
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        if (vkCreateRenderPass(context->getDevice(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }


    void RenderPipelineManager::createFramebuffers(const std::vector<VkImageView>& swapChainImageViews,
                                                     VkExtent2D swapChainExtent) {
        // Clean up old framebuffers if they exist
        for (auto framebuffer : framebuffers) {
            vkDestroyFramebuffer(context->getDevice(), framebuffer, nullptr);
        }

        // Resize framebuffers vector to match number of swap chain images
        framebuffers.resize(swapChainImageViews.size());

        // Create a framebuffer for each swap chain image view
        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            std::array<VkImageView, 2> attachments = {
                swapChainImageViews[i],    // Color attachment
                depthImageView             // Depth attachment
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(context->getDevice(), &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create framebuffer!");
            }
        }
    }


void RenderPipelineManager::createGraphicsPipeline(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts) {
    createPipelineCache();

    // Define push constant range for the vertex shader transform matrix
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4);

    // Create separate pipeline layouts for mesh and skybox

    // Mesh pipeline layout (uses meshUniformLayout and materialLayout)
        std::vector<VkDescriptorSetLayout> meshLayouts = {
            descriptorManager->getSceneLayout(),
            descriptorManager->getMaterialLayout(),
            descriptorManager->getMeshUniformLayout(),
            descriptorManager->getLightsLayout(),
        };

    VkPipelineLayoutCreateInfo meshPipelineLayoutInfo{};
    meshPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    meshPipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(meshLayouts.size());
    meshPipelineLayoutInfo.pSetLayouts = meshLayouts.data();
    meshPipelineLayoutInfo.pushConstantRangeCount = 1;
    meshPipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(context->getDevice(), &meshPipelineLayoutInfo, nullptr, &meshPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create mesh pipeline layout!");
    }

    // Skybox pipeline layout (uses only sceneLayout)
    std::vector<VkDescriptorSetLayout> skyboxLayouts = {
        descriptorManager->getSceneLayout(),
    };

    VkPipelineLayoutCreateInfo skyboxPipelineLayoutInfo{};
    skyboxPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    skyboxPipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(skyboxLayouts.size());
    skyboxPipelineLayoutInfo.pSetLayouts = skyboxLayouts.data();
    skyboxPipelineLayoutInfo.pushConstantRangeCount = 1;
    skyboxPipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(context->getDevice(), &skyboxPipelineLayoutInfo, nullptr, &skyboxPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create skybox pipeline layout!");
    }

    // Common pipeline settings
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
        vks::base::initializers::pipelineInputAssemblyStateCreateInfo(
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

    VkPipelineViewportStateCreateInfo viewportState =
        vks::base::initializers::pipelineViewportStateCreateInfo(1, 1, 0);

    VkPipelineMultisampleStateCreateInfo multisampleState =
        vks::base::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);

    std::vector<VkDynamicState> dynamicStateEnables = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState =
        vks::base::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);

    // Shared blend attachment state
    VkPipelineColorBlendAttachmentState blendAttachmentState =
        vks::base::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);

    VkPipelineColorBlendStateCreateInfo colorBlendState =
        vks::base::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

    // Load shaders
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

    // Create the mesh pipeline
    {
        // Mesh-specific rasterization state
        VkPipelineRasterizationStateCreateInfo rasterizationState =
            vks::base::initializers::pipelineRasterizationStateCreateInfo(
                VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);

        // Mesh-specific depth state
        VkPipelineDepthStencilStateCreateInfo depthStencilState =
            vks::base::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);

        // Load mesh shader stages
        shaderStages[0] = descriptorManager->getOrLoadResource<ShaderDescriptor>(
            "C:/Users/redkc/CLionProjects/ReasonableVulkan/res/shaders/hlsl/vulkanscene/mesh.vert.spv")->getShaderStage();
        shaderStages[1] = descriptorManager->getOrLoadResource<ShaderDescriptor>(
            "C:/Users/redkc/CLionProjects/ReasonableVulkan/res/shaders/hlsl/vulkanscene/mesh.frag.spv")->getShaderStage();

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
        pipelineCI.pVertexInputState = vks::MeshDescriptor::getPipelineVertexInputState({
            VertexComponent::Position,
            VertexComponent::Normal,
            VertexComponent::UV,
            VertexComponent::Color,
            VertexComponent::Tangent,
            VertexComponent::Bitangent
        });

        if (vkCreateGraphicsPipelines(context->getDevice(), pipelineCache, 1, &pipelineCI, nullptr, &pipelines.models) != VK_SUCCESS) {
            throw std::runtime_error("failed to create mesh graphics pipeline!");
        }
    }

    // Create the skybox pipeline
    {
        // Skybox-specific rasterization state
        VkPipelineRasterizationStateCreateInfo rasterizationState =
            vks::base::initializers::pipelineRasterizationStateCreateInfo(
                VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);

        // Skybox-specific depth state (depth writes disabled)
        VkPipelineDepthStencilStateCreateInfo depthStencilState =
            vks::base::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);

        // Load skybox shader stages
        shaderStages[0] = descriptorManager->getOrLoadResource<ShaderDescriptor>(
            "C:/Users/redkc/CLionProjects/ReasonableVulkan/res/shaders/hlsl/vulkanscene/skybox.vert.spv")->getShaderStage();
        shaderStages[1] = descriptorManager->getOrLoadResource<ShaderDescriptor>(
            "C:/Users/redkc/CLionProjects/ReasonableVulkan/res/shaders/hlsl/vulkanscene/skybox.frag.spv")->getShaderStage();

        // Create skybox pipeline
        VkGraphicsPipelineCreateInfo pipelineCI = vks::base::initializers::pipelineCreateInfo(
            skyboxPipelineLayout, renderPass, 0);
        pipelineCI.pInputAssemblyState = &inputAssemblyState;
        pipelineCI.pViewportState = &viewportState;
        pipelineCI.pRasterizationState = &rasterizationState;
        pipelineCI.pMultisampleState = &multisampleState;
        pipelineCI.pDepthStencilState = &depthStencilState;
        pipelineCI.pColorBlendState = &colorBlendState;
        pipelineCI.pDynamicState = &dynamicState;
        pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineCI.pStages = shaderStages.data();
        pipelineCI.pVertexInputState = vks::MeshDescriptor::getPipelineVertexInputState({
            VertexComponent::Position
        });

        if (vkCreateGraphicsPipelines(context->getDevice(), pipelineCache, 1, &pipelineCI, nullptr, &pipelines.skybox) != VK_SUCCESS) {
            throw std::runtime_error("failed to create skybox graphics pipeline!");
        }
    }
}

void RenderPipelineManager::createPipelineCache() {
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo{};
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    if (vkCreatePipelineCache(context->getDevice(), &pipelineCacheCreateInfo, nullptr, &pipelineCache) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline cache!");
    }
}

    void RenderPipelineManager::createDepthResources(VkExtent2D swapChainExtent) {
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

    if (vkCreateImage(context->getDevice(), &imageInfo, nullptr, &depthImage) != VK_SUCCESS) {
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

    if (vkAllocateMemory(context->getDevice(), &allocInfo, nullptr, &depthImageMemory) != VK_SUCCESS) {
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

    if (vkCreateImageView(context->getDevice(), &viewInfo, nullptr, &depthImageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create depth image view!");
    }
}

void RenderPipelineManager::cleanupDepthResources() {
    if (depthImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(context->getDevice(), depthImageView, nullptr);
        depthImageView = VK_NULL_HANDLE;
    }
    if (depthImage != VK_NULL_HANDLE) {
        vkDestroyImage(context->getDevice(), depthImage, nullptr);
        depthImage = VK_NULL_HANDLE;
    }
    if (depthImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(context->getDevice(), depthImageMemory, nullptr);
        depthImageMemory = VK_NULL_HANDLE;
    }
}

} // namespace vks