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
        void createShadowRenderPass();
        void createGraphicsPipeline(ShaderProgramDescriptor* shaderProgramDescriptor);
        void createShadowPipeline(ShaderProgramDescriptor* shaderProgramDescriptor);
        void createFramebuffers(VkExtent2D swapChainExtent);
        void createShadowFramebuffers();
        void createShadowResources();

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
        VkRenderPass getShadowRenderPass() const { return shadowRenderPass; }
        VkPipeline getPipeline(const boost::uuids::uuid& pipelineId) const;
        VkPipelineLayout getPipelineLayout(const boost::uuids::uuid& pipelineId) const;
        VkFramebuffer getFramebuffer(uint32_t cameraIndex, uint32_t imageIndex) const;
        VkFramebuffer getDirectionalShadowFramebuffer(uint32_t index) const { return directionalShadowFramebuffers[index]; }
        VkFramebuffer getPointShadowFramebuffer(uint32_t index) const { return pointShadowFramebuffers[index]; }
        VkFramebuffer getSpotShadowFramebuffer(uint32_t index) const { return spotShadowFramebuffers[index]; }
        bool hasPipeline(const boost::uuids::uuid& pipelineId) const { return findPipeline(pipelineId) != nullptr; }

        // Offscreen resources
        struct OffscreenTarget {
            VkImage image = VK_NULL_HANDLE;
            VkDeviceMemory memory = VK_NULL_HANDLE;
            VkImageView view = VK_NULL_HANDLE;
        };

        struct CameraResources {
            std::vector<OffscreenTarget> offscreenTargets;
            std::vector<VkFramebuffer> framebuffers;
            VkImage depthImage = VK_NULL_HANDLE;
            VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
            VkImageView depthImageView = VK_NULL_HANDLE;
        };
        std::vector<CameraResources> cameraResources;

        OffscreenTarget directionalShadows;
        OffscreenTarget pointShadows;
        OffscreenTarget spotShadows;

        std::vector<VkFramebuffer> directionalShadowFramebuffers;
        std::vector<VkFramebuffer> pointShadowFramebuffers;
        std::vector<VkFramebuffer> spotShadowFramebuffers;

        std::vector<VkImageView> directionalShadowLayerViews;
        std::vector<VkImageView> pointShadowLayerViews;
        std::vector<VkImageView> spotShadowLayerViews;

        VkFormat offscreenFormat = VK_FORMAT_R8G8B8A8_UNORM;
        VkFormat shadowFormat = VK_FORMAT_D32_SFLOAT;

        const uint32_t SHADOWMAP_DIM = 2048;
        const uint32_t MAX_DIRECTIONAL_SHADOWS = 4;
        const uint32_t MAX_POINT_SHADOWS = 4;
        const uint32_t MAX_SPOT_SHADOWS = 4;

    private:
        VulkanContext* context;
        SwapChainManager* swapChain;
        DescriptorManager* descriptorManager;

        VkRenderPass renderPass{VK_NULL_HANDLE};
        VkRenderPass shadowRenderPass{VK_NULL_HANDLE};
        VkPipelineCache pipelineCache{VK_NULL_HANDLE};

        std::vector<Pipeline> pipelines;

        // Helper methods
        void createPipelineCache();
        const Pipeline* findPipeline(const boost::uuids::uuid& pipelineId) const;
    };
}