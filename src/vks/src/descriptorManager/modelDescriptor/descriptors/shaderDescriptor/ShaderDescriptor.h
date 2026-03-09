
#ifndef SHADERHANDLE_H
#define SHADERHANDLE_H
#include "../IVulkanDescriptor.h"
#include "assetDatas/ShaderData.h"
#include <vulkan/vulkan.h>

#include "ShaderDefinesEnum.hpp"

namespace vks
{
    class ShaderDescriptor : public vks::IVulkanDescriptor {
    public:
        ShaderDescriptor(const boost::uuids::uuid& assetId, am::ShaderData& shaderData, VulkanContext& vulkanContext);
        ~ShaderDescriptor();

        VkPipelineShaderStageCreateInfo getShaderStage() const { return shaderStage; }
        void cleanup() override;

        const std::vector<ShaderDefinesEnum> getDefines() const { return defines; }
    private:

        VkShaderModule createShaderModule(const std::vector<uint32_t>& code);
        VkShaderModule shaderModule{VK_NULL_HANDLE};
        VkPipelineShaderStageCreateInfo shaderStage{};
        std::vector<ShaderDefinesEnum> defines;
        static VkShaderStageFlagBits convertShaderStage(am::ShaderStage stage);
        static std::vector<ShaderDefinesEnum> convertDefines(std::map<std::string, std::string> defines);
    };
}
#endif //SHADERHANDLE_H
