
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

        void createDepthResources(VkExtent2D swapChainExtent);
        void cleanupDepthResources();

        void cleanup();

        // Getters
        VkRenderPass getRenderPass() const { return renderPass; }
        VkPipeline getModelPipeline() const { return pipelines.models; }
        VkPipeline getSkyboxPipeline() const { return pipelines.skybox; }
        VkPipelineLayout getMeshPipelineLayout() const { return meshPipelineLayout; }
        VkPipelineLayout getSkyboxPipelineLayout() const { return skyboxPipelineLayout; }
        VkFramebuffer getFramebuffer(uint32_t index) const { return framebuffers[index]; }

        // Add these new members for depth resources
        VkImage depthImage = VK_NULL_HANDLE;
        VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
        VkImageView depthImageView = VK_NULL_HANDLE;

        struct Pipelines {
            VkPipeline models{VK_NULL_HANDLE};
            VkPipeline skybox{VK_NULL_HANDLE};
        } pipelines;

    private:
        VulkanContext* context;
        DescriptorManager* descriptorManager;

        VkRenderPass renderPass{VK_NULL_HANDLE};
        VkPipelineLayout meshPipelineLayout = VK_NULL_HANDLE;
        VkPipelineLayout skyboxPipelineLayout = VK_NULL_HANDLE;
        VkPipelineCache pipelineCache{VK_NULL_HANDLE};
        std::vector<VkFramebuffer> framebuffers;
        void createPipelineCache();
    };
}