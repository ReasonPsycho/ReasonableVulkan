
#include "ImguiManager.hpp"


#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <stdexcept>
#include <SDL3/SDL_video.h>
#include <ImGuizmo.h>

#include "../vulkanContext/VulkanContext.hpp"
#include "../swapChainManager/SwapChainManager.hpp"
#include "../renderPipelineManager/RenderPipelineManager.hpp"

namespace vks
{
    void ImguiManager::initialize(void* windowHandle, std::vector<VkImageView> swapChainImagesViews)
    {
        createDescriptorPool();
        createRenderPass();
        createPipelineCache();

        createFramebuffers(swapChainImagesViews);

        // Initialize ImGui
        IMGUI_CHECKVERSION();
        auto ctx = ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        //ImGui::StyleColorsLight();
        ImGui::StyleColorsDark();

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

        ImGuizmo::SetImGuiContext(ctx);


        createDescriptorSets(swapChainImagesViews);
    }

    void ImguiManager::createDescriptorPool()
    {
        VkDescriptorPoolSize pool_sizes[] =
        {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 },
        };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 100;
        for (VkDescriptorPoolSize& pool_size : pool_sizes)
            pool_info.maxSets += pool_size.descriptorCount;
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        if (vkCreateDescriptorPool(context->getDevice(), &pool_info, nullptr, &imguiDescriptorPool) != VK_SUCCESS)
            throw std::runtime_error("failed to create descriptor pool!");
    }

    VkDescriptorSet ImguiManager::addTexture(VkImageView imageView, VkSampler sampler)
    {
        return ImGui_ImplVulkan_AddTexture(sampler, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    VkDescriptorSet ImguiManager::getTexture(uint32_t imageIndex)
    {
        return swapChainImguiTextureIDs[imageIndex];
    }

    void ImguiManager::imguiBeginFrame()
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
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
        imguiRenderPassInfo.framebuffer = imguiFramebuffers[imageIndex];
        imguiRenderPassInfo.renderArea.offset = {0, 0};
        imguiRenderPassInfo.renderArea.extent = swapChain->getSwapChainExtent();
        VkClearValue clearValue = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        imguiRenderPassInfo.clearValueCount = 1;
        imguiRenderPassInfo.pClearValues = &clearValue;

        vkCmdBeginRenderPass(commandBuffer, &imguiRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
        vkCmdEndRenderPass(commandBuffer);
    }

    void ImguiManager::createDescriptorSets(std::vector<VkImageView> swapChainImagesViews)
    {
        swapChainImguiTextureIDs.clear();
        for (int i = 0; i < pipelineManager->offscreenTargets.size(); i++)
        {
            swapChainImguiTextureIDs.push_back(addTexture(pipelineManager->offscreenTargets[i].view, descriptorManager->defaultSampler));
        }
    }

    void ImguiManager::createFramebuffers(std::vector<VkImageView> swapChainImagesViews)
    {
        for (auto framebuffer : imguiFramebuffers)
        {
            if (framebuffer != VK_NULL_HANDLE)
                vkDestroyFramebuffer(context->getDevice(), framebuffer, nullptr);
        }
        imguiFramebuffers.clear();

        imguiFramebuffers.resize(swapChainImagesViews.size());
        for (size_t i = 0; i < swapChainImagesViews.size(); i++) {
            VkImageView attachments[] = { swapChainImagesViews[i] };
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = imguiRenderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = swapChain->getSwapChainExtent().width;
            framebufferInfo.height = swapChain->getSwapChainExtent().height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(context->getDevice(), &framebufferInfo, nullptr, &imguiFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create ImGui framebuffer!");
            }
        }
    }

    void ImguiManager::createPipelineCache()
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

    void ImguiManager::createRenderPass()
{
    std::array<VkAttachmentDescription, 1> attachments = {};

    // Color attachment (Swapchain)
    attachments[0].format = swapChain->getSwapChainImageFormat();
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;  // Clear the swapchain image to remove "mirages"
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // We are clearing it, so UNDEFINED is better
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

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

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(context->getDevice(), &renderPassInfo, nullptr, &imguiRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create ImGui render pass!");
    }
}
    void ImguiManager::cleanup()
    {
        for (auto framebuffer : imguiFramebuffers)
        {
            if (framebuffer != VK_NULL_HANDLE)
                vkDestroyFramebuffer(context->getDevice(), framebuffer, nullptr);
        }
        imguiFramebuffers.clear();

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
