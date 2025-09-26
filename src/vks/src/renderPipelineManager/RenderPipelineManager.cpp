
#include "RenderPipelineManager.hpp"
#include <stdexcept>

#include "../descriptorManager/modelDescriptor/descriptors/meshDescriptor/MeshDescriptor.h"
#include "../descriptorManager/modelDescriptor/descriptors/shaderDescriptor/ShaderDescriptor.h"

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

        if (pipelines.models != VK_NULL_HANDLE) {
            vkDestroyPipeline(context->getDevice(), pipelines.models, nullptr);
        }
        if (pipelines.skybox != VK_NULL_HANDLE) {
            vkDestroyPipeline(context->getDevice(), pipelines.skybox, nullptr);
        }


        if (pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(context->getDevice(), pipelineLayout, nullptr);
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

        // Pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

        if (vkCreatePipelineLayout(context->getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::base::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
        VkPipelineRasterizationStateCreateInfo rasterizationState = vks::base::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE,0);
        VkPipelineColorBlendAttachmentState blendAttachmentState = vks::base::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
        VkPipelineColorBlendStateCreateInfo colorBlendState = vks::base::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
        VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::base::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
        VkPipelineViewportStateCreateInfo viewportState = vks::base::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
        VkPipelineMultisampleStateCreateInfo multisampleState = vks::base::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
        std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicState = vks::base::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
        VkGraphicsPipelineCreateInfo pipelineCI = vks::base::initializers::pipelineCreateInfo(pipelineLayout, renderPass, 0);
        pipelineCI.pInputAssemblyState = &inputAssemblyState;
        pipelineCI.pRasterizationState = &rasterizationState;
        pipelineCI.pColorBlendState = &colorBlendState;
        pipelineCI.pMultisampleState = &multisampleState;
        pipelineCI.pViewportState = &viewportState;
        pipelineCI.pDepthStencilState = &depthStencilState;
        pipelineCI.pDynamicState = &dynamicState;
        pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineCI.pStages = shaderStages.data();
        pipelineCI.pVertexInputState = vks::MeshDescriptor::getPipelineVertexInputState({VertexComponent::Position, VertexComponent::Normal, VertexComponent::UV, VertexComponent::Color, VertexComponent::Tangent, VertexComponent::Bitangent});;

        // Default mesh rendering pipeline
        shaderStages[0] = descriptorManager->getOrLoadResource<ShaderDescriptor>("vulkanscene/mesh.vert.spv")->getShaderStage();
        shaderStages[1] =  descriptorManager->getOrLoadResource<ShaderDescriptor>("vulkanscene/mesh.frag.spv")->getShaderStage();
        if (vkCreateGraphicsPipelines(context->getDevice(), pipelineCache, 1, &pipelineCI, nullptr, &pipelines.models)!= VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        // Pipeline for the skybox
        rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
        depthStencilState.depthWriteEnable = VK_FALSE;

        shaderStages[0] =  descriptorManager->getOrLoadResource<ShaderDescriptor>("vulkanscene/skybox.vert.spv")->getShaderStage();
        shaderStages[1] =         descriptorManager->getOrLoadResource<ShaderDescriptor>("vulkanscene/skybox.frag.spv")->getShaderStage();
        if (vkCreateGraphicsPipelines(context->getDevice(), pipelineCache, 1, &pipelineCI, nullptr, &pipelines.skybox) != VK_SUCCESS){
        throw std::runtime_error("failed to create graphics pipeline!");
        }
    }

void RenderPipelineManager::createPipelineCache() {
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo{};
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    if (vkCreatePipelineCache(context->getDevice(), &pipelineCacheCreateInfo, nullptr, &pipelineCache) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline cache!");
    }
}

} // namespace vks