//
// Created by redkc on 10/08/2025.
//

#ifndef MATERIAL_H
#define MATERIAL_H
#include <vulkan/vulkan_core.h>

#include "../IVulkanDescriptor.h"
#include "../textureDescriptor/TextureDescriptor.h"
#include "assetDatas/MaterialData.h"
#include "../../../buffers/MaterialSSBO.hpp"


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

        uint32_t baseColorTexIndex{0};
        uint32_t normalTexIndex{0};
        uint32_t metallicRoughnessTexIndex{0};
        uint32_t occlusionTexIndex{0};
        uint32_t emissiveTexIndex{0};

        uint32_t materialIndex{0};

        TextureDescriptor *baseColorTexture = nullptr;
        TextureDescriptor *metallicRoughnessTexture = nullptr;
        TextureDescriptor *normalTexture = nullptr;
        TextureDescriptor *occlusionTexture = nullptr;
        TextureDescriptor *emissiveTexture = nullptr;

        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        uint32_t descriptorBindingFlags{0};
        DescriptorManager* assetHandleManager = nullptr;

        MaterialDescriptor(const boost::uuids::uuid& assetId, DescriptorManager* assetHandleManager, am::MaterialData& materialData,VulkanContext& vulkanContext);

        ~MaterialDescriptor();

        void update(am::MaterialData& materialData);
        GPUMaterialData getGPUMaterialData() const;
        void setUpDescriptorSet(VkDescriptorSetLayout materialLayout, VkDescriptorPool materialDescriptorPool, VkDescriptorImageInfo defaultImageInfo, VkDescriptorImageInfo defaultCubeImageInfo) {};
        void cleanup() override {};

    };
}


#endif //MATERIAL_H
