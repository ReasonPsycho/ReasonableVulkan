
#pragma once
#include <memory>
#include <vulkan/vulkan_core.h>

#include "include/GraphicsEngine.hpp"

namespace am
{
	class AssetManagerInterface;
}

namespace vks {
	class RenderManager;
	class RenderPipelineManager;
	class DescriptorManager;
	class SwapChainManager;
	class VulkanContext;

	class VulkanRenderer : public gfx::GraphicsEngine {
	public:
		explicit VulkanRenderer(am::AssetManagerInterface* assetManagerInterface);
		~VulkanRenderer();

		void loadModel(boost::uuids::uuid uuid) override;
		void loadShader(boost::uuids::uuid uuid) override;
		void drawModel(boost::uuids::uuid uuid, const glm::mat4& transform) override;
		void renderFrame() override;
		void render() override;
		void endFrame() override;

		void initialize(void* windowHandle, uint32_t width, uint32_t height);
		void cleanup();
		void waitIdle();
		void handleWindowResize(uint32_t width, uint32_t height);

	private:
		std::unique_ptr<VulkanContext> context;
		std::unique_ptr<SwapChainManager> swapChain;
		std::unique_ptr<DescriptorManager> descriptorManager;
		std::unique_ptr<RenderPipelineManager> pipelineManager;
		std::unique_ptr<RenderManager> renderManager;

		// Synchronization objects
		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence> inFlightFences;
		uint32_t currentFrame = 0;
		static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

		// Command pool and buffers
		VkCommandPool commandPool{VK_NULL_HANDLE};
		std::vector<VkCommandBuffer> commandBuffers;

		void createSyncObjects();
		void createCommandPool();
		void createCommandBuffers();
	};
}
