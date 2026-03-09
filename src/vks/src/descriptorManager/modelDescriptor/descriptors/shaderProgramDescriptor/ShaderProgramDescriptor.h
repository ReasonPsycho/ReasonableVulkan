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
        ShaderProgramDescriptor(am::ShaderProgramData& programData, DescriptorManager* descriptorManager, VulkanContext& vulkanContext);
        ~ShaderProgramDescriptor() override;

        void cleanup() override;

        const std::vector<VkPipelineShaderStageCreateInfo>& getShaderStages() const { return shaderStages; }
        const std::vector<ShaderDefinesEnum>& getCombinedDefines() const { return combinedDefines; }

    private:
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
        std::vector<ShaderDefinesEnum> combinedDefines;
        
        // We don't own the individual ShaderDescriptors, the DescriptorManager does.
        // But we need to make sure they are loaded.
    };
}

#endif // SHADERPROGRAMDESCRIPTOR_H
