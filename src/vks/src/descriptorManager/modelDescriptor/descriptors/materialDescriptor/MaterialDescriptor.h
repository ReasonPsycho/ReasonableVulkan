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
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        uint32_t descriptorBindingFlags;

        MaterialDescriptor( am::MaterialData& materialData,VkDescriptorSetLayout materialLayout, vks::base::VulkanDevice& device, VkQueue copyQueue);

        ~MaterialDescriptor();

        void createDescriptorPool();
        void createDescriptorSet(VkDescriptorSetLayout materialLayout);
        void cleanup() override {};

    private:
        void setupDescriptors(VkDescriptorSetLayout materialLayout);
    };
}


#endif //MATERIAL_H
