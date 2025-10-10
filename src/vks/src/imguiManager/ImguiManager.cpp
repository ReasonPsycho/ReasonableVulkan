
#include "ImguiManager.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <stdexcept>
#include <SDL3/SDL_video.h>

#include "../vulkanContext/VulkanContext.hpp"
#include "../swapChainManager/SwapChainManager.hpp"
#include "../renderPipelineManager/RenderPipelineManager.hpp"

namespace vks
{
    void vks::ImguiManager::initialize(void* windowHandle)
    {
        createDescriptorPool();
        createRenderPass();
        createPipelineCache();

        // Initialize ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::GetStyle().WindowRounding = 0.0f;
            ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

        // Create command buffer for ImGui rendering
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = context->getCommandPool(QueueType::Graphics);
        allocInfo.commandBufferCount = 1;
        if (vkAllocateCommandBuffers(context->getDevice(), &allocInfo, &imguiCommandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate ImGui command buffer!");
        }

        int width, height;
        SDL_Window* window = reinterpret_cast<SDL_Window*>(windowHandle);
        SDL_GetWindowSizeInPixels(window, &width, &height);
        io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));

        // Setup Platform/Renderer backends
        ImGui_ImplSDL3_InitForVulkan(reinterpret_cast<SDL_Window*>(windowHandle));
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.ApiVersion = VK_API_VERSION_1_3;
        init_info.Instance = context->getInstance();
        init_info.PhysicalDevice = context->getPhysicalDevice();
        init_info.Device = context->getDevice();
        init_info.QueueFamily = context->getQueueFamilyIndices().graphics;
        init_info.Queue = context->getGraphicsQueue();
        init_info.RenderPass = imguiRenderPass;
        init_info.PipelineCache = imguiPipelineCache;
        init_info.DescriptorPool = imguiDescriptorPool;
        init_info.MinImageCount = 2;
        init_info.ImageCount = 2;
        init_info.Allocator = nullptr;
        init_info.CheckVkResultFn = nullptr;

        ImGui_ImplVulkan_Init(&init_info);

        // Upload ImGui Fonts
        VkCommandBuffer command_buffer = context->beginSingleTimeCommands();
        ImGui_ImplVulkan_CreateFontsTexture();
        context->endSingleTimeCommands(command_buffer);
    }

    void ImguiManager::createDescriptorPool()
    {
        VkDescriptorPoolSize pool_sizes[] =
        {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE },
        };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 0;
        for (VkDescriptorPoolSize& pool_size : pool_sizes)
            pool_info.maxSets += pool_size.descriptorCount;
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        if (vkCreateDescriptorPool(context->getDevice(), &pool_info, nullptr, &imguiDescriptorPool) != VK_SUCCESS)
            throw std::runtime_error("failed to create descriptor pool!");
    }

    void ImguiManager::imguiBeginFrame()
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }

    void ImguiManager::imguiEndFrame()
    {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::Render();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable){
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }

    void ImguiManager::imguiRenderFrame(VkCommandBuffer commandBuffer,uint32_t imageIndex)
    {
        // Remove ImGui::Render() from here - it should be in endFrame()
        VkRenderPassBeginInfo imguiRenderPassInfo{};
        imguiRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        imguiRenderPassInfo.renderPass = imguiRenderPass;
        imguiRenderPassInfo.framebuffer = pipelineManager->getFramebuffer(imageIndex);
        imguiRenderPassInfo.renderArea.offset = {0, 0};
        imguiRenderPassInfo.renderArea.extent = swapChain->getSwapChainExtent();
        imguiRenderPassInfo.clearValueCount = 0;
        imguiRenderPassInfo.pClearValues = nullptr;

        vkCmdBeginRenderPass(commandBuffer, &imguiRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
        vkCmdEndRenderPass(commandBuffer);
    }

    void vks::ImguiManager::createPipelineCache()
    {
        VkPipelineCacheCreateInfo pipelineCacheInfo{};
        pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

        if (vkCreatePipelineCache(context->getDevice(), &pipelineCacheInfo, nullptr, &imguiPipelineCache) != VK_SUCCESS) {
            throw std::runtime_error("failed to create ImGui pipeline cache!");
        }
    }


    ImguiManager::ImguiManager(vks::VulkanContext* context, vks::SwapChainManager* swapChain,
        vks::RenderPipelineManager* pipelineManager, vks::DescriptorManager* descriptorManager) : context(context),
        swapChain(swapChain),
        pipelineManager(pipelineManager), descriptorManager(
            descriptorManager)
    {
    }

    void vks::ImguiManager::createRenderPass()
{
    std::array<VkAttachmentDescription, 2> attachments = {};

    // Color attachment
    attachments[0].format = swapChain->getSwapChainImageFormat();
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;  // Load existing content
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // Initial layout matches the previous pass's final layout
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Depth attachment
    attachments[1].format = VK_FORMAT_D32_SFLOAT;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;  // Load existing depth
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 0;
    renderPassInfo.pDependencies = nullptr;

    if (vkCreateRenderPass(context->getDevice(), &renderPassInfo, nullptr, &imguiRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create ImGui render pass!");
    }
}    void vks::ImguiManager::cleanup()
    {
        if (imguiFramebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(context->getDevice(), imguiFramebuffer, nullptr);
        }
        if (imguiRenderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(context->getDevice(), imguiRenderPass, nullptr);
        }
        if (imguiPipelineCache != VK_NULL_HANDLE) {
            vkDestroyPipelineCache(context->getDevice(), imguiPipelineCache, nullptr);
        }
        if (imguiCommandBuffer != VK_NULL_HANDLE) {
            vkFreeCommandBuffers(context->getDevice(), context->getCommandPool(QueueType::Graphics), 1, &imguiCommandBuffer);
        }
        if (imguiDescriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(context->getDevice(), imguiDescriptorPool, nullptr);
        }

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }
}
