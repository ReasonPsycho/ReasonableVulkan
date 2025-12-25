
#pragma once
#include <memory>
#include <glm/detail/type_mat4x4.hpp>
#include <vulkan/vulkan_core.h>

#include "include/GraphicsEngine.hpp"
#include "src/imguiManager/ImguiManager.hpp"
#include "PlatformInterface.hpp"


namespace am
{
	class AssetManagerInterface;
}

class PlatformInterface;

namespace vks {
	class RenderManager;
	class RenderPipelineManager;
	class DescriptorManager;
	class SwapChainManager;
	class VulkanContext;

	class VulkanRenderer : public gfx::GraphicsEngine {
	public:
		explicit VulkanRenderer(am::AssetManagerInterface* assetManagerInterface);
		~VulkanRenderer() override;

		void setCameraData(const glm::mat4& projection, const glm::mat4& view, const glm::vec3 cameraPos) override;
		void loadModel(boost::uuids::uuid uuid) override;
		void loadShader(boost::uuids::uuid uuid) override;
		void drawModel(boost::uuids::uuid uuid, const glm::mat4& transform) override;
		void drawLight(gfx::PointLightData pointLightData, const glm::mat4& transform) override;
		void drawLight(gfx::SpotLightData spotLightData, const glm::mat4& transform) override;
		void drawLight(gfx::DirectionalLightData directionalLightData, const glm::mat4& transform) override;

		void beginFrame() override;
		void renderFrame() override;
		void endFrame() override;

		void initialize(plt::PlatformInterface* platform, uint32_t width, uint32_t height) override;

		void cleanup();
		void waitIdle();
		void handleWindowResize(uint32_t width, uint32_t height);

	private:
		std::unique_ptr<VulkanContext> context;
		std::unique_ptr<SwapChainManager> swapChain;
		std::unique_ptr<DescriptorManager> descriptorManager;
		std::unique_ptr<RenderPipelineManager> pipelineManager;
		std::unique_ptr<RenderManager> renderManager;
		plt::PlatformInterface* platformInterface;

		bool minimized = false;
#if ENABLE_IMGUI
		std::unique_ptr<ImguiManager> imguiManager;
#endif
	};
}