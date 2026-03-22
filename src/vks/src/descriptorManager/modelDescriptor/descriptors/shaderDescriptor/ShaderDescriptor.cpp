
#include "ShaderDescriptor.h"
#include <cassert>
#include "../../../../base/VulkanDevice.h"
namespace vks
{
    ShaderDescriptor::ShaderDescriptor(const boost::uuids::uuid& assetId, am::ShaderData& shaderData,VulkanContext& vulkanContext)
        : IVulkanDescriptor(assetId, vulkanContext) {

        // Create shader module from bytecode
        shaderModule = createShaderModule(shaderData.bytecode);
        assert(shaderModule != VK_NULL_HANDLE);

        defines = convertDefines(shaderData.defines);

        // Setup shader stage info
        shaderStage = {};
        shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStage.stage = convertShaderStage(shaderData.stage);
        shaderStage.module = shaderModule;
        shaderStage.pName = "main";  // Assuming main entry point
    }

    ShaderDescriptor::~ShaderDescriptor() {
        cleanup();
    }

    void ShaderDescriptor::cleanup() {
        if (shaderModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(device, shaderModule, nullptr);
            shaderModule = VK_NULL_HANDLE;
        }
    }

    VkShaderModule ShaderDescriptor::createShaderModule(const std::vector<uint32_t>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.codeSize = code.size() * sizeof(uint32_t);
        createInfo.pCode = code.data();

        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shader module!");
        }

        return shaderModule;
    }

    VkShaderStageFlagBits ShaderDescriptor::convertShaderStage(am::ShaderStage stage) {
        switch (stage) {
        case am::ShaderStage::Vertex:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case am::ShaderStage::Fragment:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case am::ShaderStage::Compute:
            return VK_SHADER_STAGE_COMPUTE_BIT;
        case am::ShaderStage::Geometry:
            return VK_SHADER_STAGE_GEOMETRY_BIT;
        case am::ShaderStage::TessellationControl:
            return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case am::ShaderStage::TessellationEvaluation:
            return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        default:
            throw std::runtime_error("Unknown shader stage");
        }
    }

    std::vector<ShaderDefinesEnum> ShaderDescriptor::convertDefines(std::map<std::string, std::string> defines)
    {
        std::vector<ShaderDefinesEnum> result;
       for (auto& [key, value] : defines)
       {
           if (key == "SCENE_UBO_GLSL") {
               result.push_back(ShaderDefinesEnum::SCENE_UBO_GLSL);
           } else if (key == "VERTEX_IO_GLSL")  {
               result.push_back(ShaderDefinesEnum::VERTEX_IO_GLSL);
           }else if (key == "LIGHTING_COMMON_GLSL")  {
               result.push_back(ShaderDefinesEnum::LIGHTING_COMMON_GLSL);
           }else if (key == "MODEL_PC_GLSL")  {
               result.push_back(ShaderDefinesEnum::MODEL_PC_GLSL);
           }else if (key == "MATERIAL_PBR_GLSL")  {
               result.push_back(ShaderDefinesEnum::MATERIAL_PBR_GLSL);
           }else if (key == "MATERIAL_SKYBOX_GLSL")  {
               result.push_back(ShaderDefinesEnum::MATERIAL_SKYBOX_GLSL);
           }else if (key == "WIREMESH_GLSL")  {
               result.push_back(ShaderDefinesEnum::WIREMESH_GLSL);
           }else if (key == "LIGHTSPACEMATRIX_PC")  {
               result.push_back(ShaderDefinesEnum::LIGHTSPACEMATRIX_PC);
           }else if (key == "CUBELIGHTSPACEMATRIX_PC")  {
               result.push_back(ShaderDefinesEnum::CUBELIGHTSPACEMATRIX_PC);
           }
       }
        return result;
    }
}