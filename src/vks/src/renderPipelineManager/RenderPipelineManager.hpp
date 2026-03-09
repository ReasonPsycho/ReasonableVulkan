#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include "../vulkanContext/VulkanContext.hpp"
#include "../descriptorManager/DescriptorManager.h"

namespace vks {
    class ShaderProgramDescriptor;

    class RenderPipelineManager {
    public:
        RenderPipelineManager(VulkanContext* context, DescriptorManager* descriptorManager);
        ~RenderPipelineManager();

        void createRenderPass();
        void createGraphicsPipeline(const std::string& pipelineId, ShaderProgramDescriptor* shaderProgramDescriptor);
        void createFramebuffers(const std::vector<VkImageView>& swapChainImageViews,
                               VkExtent2D swapChainExtent);

        void createDepthResources(VkExtent2D swapChainExtent);
        void cleanupDepthResources();

        void cleanup();

        // Pipeline structure to hold pipeline data
        struct Pipeline {
            std::string id;
            VkPipeline handle{VK_NULL_HANDLE};
            VkPipelineLayout layout{VK_NULL_HANDLE};
        };

        // Getters
        VkRenderPass getRenderPass() const { return renderPass; }
        VkPipeline getPipeline(const std::string& pipelineId) const;
        VkPipelineLayout getPipelineLayout(const std::string& pipelineId) const;
        VkFramebuffer getFramebuffer(uint32_t index) const { return framebuffers[index]; }


        // Add these new members for depth resources
        VkImage depthImage = VK_NULL_HANDLE;
        VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
        VkImageView depthImageView = VK_NULL_HANDLE;

    private:
        VulkanContext* context;
        DescriptorManager* descriptorManager;

        VkRenderPass renderPass{VK_NULL_HANDLE};
        VkPipelineCache pipelineCache{VK_NULL_HANDLE};
        std::vector<VkFramebuffer> framebuffers;
        std::vector<Pipeline> pipelines;

        // Helper methods
        void createPipelineCache();
        const Pipeline* findPipeline(const std::string& pipelineId) const;
    };
}