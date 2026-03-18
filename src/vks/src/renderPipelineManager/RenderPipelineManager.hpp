#pragma once
#include <boost/uuid/uuid.hpp>
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include "../vulkanContext/VulkanContext.hpp"
#include "../descriptorManager/DescriptorManager.h"

namespace vks {
    class ShaderProgramDescriptor;
    class SwapChainManager;

    class RenderPipelineManager {
    public:
        RenderPipelineManager(VulkanContext* context, SwapChainManager* swapChain, DescriptorManager* descriptorManager);
        ~RenderPipelineManager();

        void createRenderPass();
        void createGraphicsPipeline(ShaderProgramDescriptor* shaderProgramDescriptor);
        void createFramebuffers(VkExtent2D swapChainExtent);

        void createDepthResources(VkExtent2D swapChainExtent);
        void cleanupDepthResources();

        void createOffscreenResources(VkExtent2D extent);
        void cleanupOffscreenResources();

        void cleanup();

        // Pipeline structure to hold pipeline data
        struct Pipeline {
            boost::uuids::uuid id;
            VkPipeline handle{VK_NULL_HANDLE};
            VkPipelineLayout layout{VK_NULL_HANDLE};
        };

        // Getters
        VkRenderPass getRenderPass() const { return renderPass; }
        VkPipeline getPipeline(const boost::uuids::uuid& pipelineId) const;
        VkPipelineLayout getPipelineLayout(const boost::uuids::uuid& pipelineId) const;
        VkFramebuffer getFramebuffer(uint32_t index) const { return framebuffers[index]; }

        // Offscreen resources
        struct OffscreenTarget {
            VkImage image = VK_NULL_HANDLE;
            VkDeviceMemory memory = VK_NULL_HANDLE;
            VkImageView view = VK_NULL_HANDLE;
        };
        std::vector<OffscreenTarget> offscreenTargets;
        VkFormat offscreenFormat = VK_FORMAT_B8G8R8A8_SRGB;

        // Add these new members for depth resources
        VkImage depthImage = VK_NULL_HANDLE;
        VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
        VkImageView depthImageView = VK_NULL_HANDLE;

    private:
        VulkanContext* context;
        SwapChainManager* swapChain;
        DescriptorManager* descriptorManager;

        VkRenderPass renderPass{VK_NULL_HANDLE};
        VkPipelineCache pipelineCache{VK_NULL_HANDLE};
        std::vector<VkFramebuffer> framebuffers;

        std::vector<Pipeline> pipelines;

        // Helper methods
        void createPipelineCache();
        const Pipeline* findPipeline(const boost::uuids::uuid& pipelineId) const;
    };
}