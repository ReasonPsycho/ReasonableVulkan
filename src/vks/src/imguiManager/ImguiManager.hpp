//
// Created by redkc on 09/10/2025.
//

#ifndef REASONABLEVULKAN_IMGUIMANAGER_HPP
#define REASONABLEVULKAN_IMGUIMANAGER_HPP
#include <vulkan/vulkan_core.h>

namespace vks
{
    class VulkanContext;
    class SwapChainManager;
    class DescriptorManager;
    class RenderPipelineManager;

    class ImguiManager
    {
    public:
        ImguiManager(vks::VulkanContext* context,
                     vks::SwapChainManager* swapChain,
                     vks::RenderPipelineManager* pipelineManager,
                     vks::DescriptorManager* descriptorManager);


        // Core Vulkan components
        vks::VulkanContext* context;
        vks::SwapChainManager* swapChain;
        vks::RenderPipelineManager* pipelineManager;
        vks::DescriptorManager* descriptorManager;

        void createRenderPass();
        void cleanup();
        void createPipelineCache();
        void initialize(void* windowHandle);
        void createDescriptorPool();
        void imguiBeginFrame();
        void imguiEndFrame();
        void imguiRenderFrame(VkCommandBuffer commandBuffer,uint32_t imageIndex);
        VkDescriptorPool imguiDescriptorPool{VK_NULL_HANDLE};
        VkCommandBuffer imguiCommandBuffer{VK_NULL_HANDLE};
        VkPipelineCache imguiPipelineCache{VK_NULL_HANDLE};
        VkRenderPass imguiRenderPass{VK_NULL_HANDLE};
        VkFramebuffer imguiFramebuffer{VK_NULL_HANDLE};
    };
}
#endif //REASONABLEVULKAN_IMGUIMANAGER_HPP