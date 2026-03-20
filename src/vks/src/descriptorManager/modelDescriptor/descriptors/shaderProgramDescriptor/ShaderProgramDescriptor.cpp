#include "../shaderProgramDescriptor/ShaderProgramDescriptor.h"
#include <algorithm>
#include "../../../DescriptorManager.h"

namespace vks {

    ShaderProgramDescriptor::ShaderProgramDescriptor(const boost::uuids::uuid& assetId, am::ShaderProgramData& programData, DescriptorManager* descriptorManager, VulkanContext& vulkanContext)
        : IVulkanDescriptor(assetId, vulkanContext) {
        
        auto processStage = [&](std::shared_ptr<am::AssetInfo> stageInfo) {
            if (stageInfo) {
                auto shaderDesc = descriptorManager->getOrLoadResource<ShaderDescriptor>(stageInfo->id);
                if (shaderDesc) {
                    shaderStages.push_back(shaderDesc->getShaderStage());
                    auto stageDefines = shaderDesc->getDefines();
                    defines.insert(defines.end(), stageDefines.begin(), stageDefines.end());
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
        std::ranges::sort(defines);
        auto [first, last] = std::ranges::unique(defines);
        defines.erase(first, last);
    }

    ShaderProgramDescriptor::~ShaderProgramDescriptor() {
        cleanup();
    }

    void ShaderProgramDescriptor::cleanup() {
        // Individual ShaderDescriptors are managed by DescriptorManager
        shaderStages.clear();
        defines.clear();
    }

} // namespace vks
