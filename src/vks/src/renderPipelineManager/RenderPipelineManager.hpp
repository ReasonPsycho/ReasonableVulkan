
#pragma once
#include <vulkan/vulkan.h>
#include "../vulkanContext/VulkanContext.hpp"
#include "../descriptorManager/DescriptorManager.h"

namespace vks {
    class RenderPipelineManager {
    public:
        RenderPipelineManager(VulkanContext* context, DescriptorManager* descriptorManager);
        ~RenderPipelineManager();

        void createRenderPass();
        void createGraphicsPipeline(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
        void createFramebuffers(const std::vector<VkImageView>& swapChainImageViews,
                               VkExtent2D swapChainExtent);

        void cleanup();

        // Getters
        VkRenderPass getRenderPass() const { return renderPass; }
        VkPipeline getModelPipeline() const { return pipelines.models; }
        VkPipeline getSkyboxPipeline() const { return pipelines.skybox; }
        VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }
        VkFramebuffer getFramebuffer(uint32_t index) const { return framebuffers[index]; }


        struct Pipelines {
            VkPipeline models{VK_NULL_HANDLE};
            VkPipeline skybox{VK_NULL_HANDLE};
        } pipelines;

    private:
        VulkanContext* context;
        DescriptorManager* descriptorManager;

        VkRenderPass renderPass{VK_NULL_HANDLE};
        VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};
        VkPipelineCache pipelineCache{VK_NULL_HANDLE};
        std::vector<VkFramebuffer> framebuffers;
        VkImageView depthImageView{VK_NULL_HANDLE};

        void createPipelineCache();
        VkShaderModule createShaderModule(const std::vector<char>& code);
    };
}