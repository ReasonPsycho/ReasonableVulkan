//
// Created by redkc on 10/08/2025.
//

#ifndef MATERIAL_H
#define MATERIAL_H
#include <vulkan/vulkan_core.h>

#include "../IVulkanDescriptor.h"
#include "../textureDescriptor/TextureDescriptor.h"
#include "assetDatas/MaterialData.h"


namespace vks
{
    class DescriptorManager;

    namespace base
    {
        struct VulkanDevice;
    }

    enum DescriptorBindingFlags {
        ImageBaseColor = 0x00000001,
        ImageNormalMap = 0x00000002
    };

    struct MaterialDescriptor : IVulkanDescriptor {
        float alphaCutoff = 1.0f;
        float metallicFactor = 1.0f;
        float roughnessFactor = 1.0f;
        glm::vec4 baseColorFactor = glm::vec4(1.0f);
        TextureDescriptor *baseColorTexture = nullptr;
        TextureDescriptor *metallicRoughnessTexture = nullptr;
        TextureDescriptor *normalTexture = nullptr;
        TextureDescriptor *occlusionTexture = nullptr;
        TextureDescriptor *emissiveTexture = nullptr;

        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        uint32_t descriptorBindingFlags;

        MaterialDescriptor(DescriptorManager* assetHandleManager, am::MaterialData& materialData,VulkanContext& vulkanContext);

        ~MaterialDescriptor();

        void setUpDescriptorSet(VkDescriptorSetLayout materialLayout, VkDescriptorPool materialDescriptorPool, VkDescriptorImageInfo defaultImageInfo);
        void cleanup() override {};

    };
}


#endif //MATERIAL_H
