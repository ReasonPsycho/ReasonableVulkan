//
// Created by redkc on 10/08/2025.
//

#ifndef MATERIAL_H
#define MATERIAL_H
#include <vulkan/vulkan_core.h>

#include "../IVulkanHandle.h"
#include "../textureHandle/TextureHandle.h"
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

    struct MaterialHandle : IVulkanHandle {
        float alphaCutoff = 1.0f;
        float metallicFactor = 1.0f;
        float roughnessFactor = 1.0f;
        glm::vec4 baseColorFactor = glm::vec4(1.0f);
        TextureHandle *baseColorTexture = nullptr;
        TextureHandle *metallicRoughnessTexture = nullptr;
        TextureHandle *normalTexture = nullptr;
        TextureHandle *occlusionTexture = nullptr;
        TextureHandle *emissiveTexture = nullptr;

        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        static VkDescriptorSetLayout descriptorSetLayoutImage;
        uint32_t descriptorBindingFlags;

        MaterialHandle(base::VulkanDevice *device, am::MaterialData& materialData, VkQueue copyQueue);

        ~MaterialHandle();

        void createDescriptorPool();
        void createDescriptorSetLayout();
        void createDescriptorSet();
        void cleanup() override {};

    private:
        void setupDescriptors();
    };
}


#endif //MATERIAL_H
