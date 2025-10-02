
#ifndef SHADERHANDLE_H
#define SHADERHANDLE_H
#include "../IVulkanDescriptor.h"
#include "assetDatas/ShaderData.h"
#include <vulkan/vulkan.h>

namespace vks
{
    class ShaderDescriptor : public vks::IVulkanDescriptor {
    public:
        ShaderDescriptor(am::ShaderData& shaderData, VulkanContext& vulkanContext);
        ~ShaderDescriptor();

        VkPipelineShaderStageCreateInfo getShaderStage() const { return shaderStage; }
        void cleanup() override;

    private:
        VkShaderModule createShaderModule(const std::vector<uint32_t>& code);
        VkShaderModule shaderModule{VK_NULL_HANDLE};
        VkPipelineShaderStageCreateInfo shaderStage{};
        static VkShaderStageFlagBits convertShaderStage(am::ShaderStage stage);
    };
}
#endif //SHADERHANDLE_H
