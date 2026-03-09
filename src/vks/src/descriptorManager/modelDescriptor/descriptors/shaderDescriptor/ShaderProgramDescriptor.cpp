#include "ShaderProgramDescriptor.h"
#include <algorithm>
#include "../../../DescriptorManager.h"

namespace vks {

    ShaderProgramDescriptor::ShaderProgramDescriptor(am::ShaderProgramData& programData, DescriptorManager* descriptorManager, VulkanContext& vulkanContext)
        : IVulkanDescriptor(vulkanContext) {
        
        auto processStage = [&](std::shared_ptr<am::AssetInfo> stageInfo) {
            if (stageInfo) {
                auto shaderDesc = descriptorManager->getOrLoadResource<ShaderDescriptor>(stageInfo->id);
                if (shaderDesc) {
                    shaderStages.push_back(shaderDesc->getShaderStage());
                    auto stageDefines = shaderDesc->getDefines();
                    combinedDefines.insert(combinedDefines.end(), stageDefines.begin(), stageDefines.end());
                }
            }
        };

        processStage(programData.vertexShader);
        processStage(programData.fragmentShader);
        processStage(programData.computeShader);
        processStage(programData.geometryShader);
        processStage(programData.tessellationControlShader);
        processStage(programData.tessellationEvaluationShader);

        // Deduplicate defines
        std::ranges::sort(combinedDefines);
        auto [first, last] = std::ranges::unique(combinedDefines);
        combinedDefines.erase(first, last);
    }

    ShaderProgramDescriptor::~ShaderProgramDescriptor() {
        cleanup();
    }

    void ShaderProgramDescriptor::cleanup() {
        // Individual ShaderDescriptors are managed by DescriptorManager
        shaderStages.clear();
        combinedDefines.clear();
    }

} // namespace vks
