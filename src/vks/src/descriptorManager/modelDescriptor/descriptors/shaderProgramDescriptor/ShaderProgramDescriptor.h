#ifndef SHADERPROGRAMDESCRIPTOR_H
#define SHADERPROGRAMDESCRIPTOR_H

#include "../IVulkanDescriptor.h"
#include "assetDatas/ShaderProgramData.h"
#include "../shaderDescriptor/ShaderDescriptor.h"
#include <vector>
#include <memory>

namespace vks {
    class DescriptorManager;

    class ShaderProgramDescriptor : public IVulkanDescriptor {
    public:
        ShaderProgramDescriptor(const boost::uuids::uuid& assetId, am::ShaderProgramData& programData, DescriptorManager* descriptorManager, VulkanContext& vulkanContext);
        ~ShaderProgramDescriptor() override;

        void cleanup() override;

        const std::vector<VkPipelineShaderStageCreateInfo>& getShaderStages() const { return shaderStages; }
        const std::vector<ShaderDefinesEnum>& getDefines() const { return defines; }

    private:
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
        std::vector<ShaderDefinesEnum> defines;
    };
}

#endif // SHADERPROGRAMDESCRIPTOR_H
