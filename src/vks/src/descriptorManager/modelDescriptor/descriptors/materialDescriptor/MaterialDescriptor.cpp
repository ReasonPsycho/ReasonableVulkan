//
// Created by redkc on 10/08/2025.
//

#include "MaterialDescriptor.h"

#include "Asset.hpp"
#include "../../../DescriptorManager.h"
#
// Static member definition


vks::MaterialDescriptor::MaterialDescriptor(const boost::uuids::uuid& assetId, DescriptorManager* assetHandleManager, am::MaterialData& materialData,VulkanContext& vulkanContext) : IVulkanDescriptor(assetId, vulkanContext)
{
    this->assetHandleManager = assetHandleManager;
    // Initialize numeric fields
    alphaCutoff = materialData.alphaCutoff;
    metallicFactor = materialData.metallicFactor;
    roughnessFactor = materialData.roughnessFactor;
    baseColorFactor = materialData.baseColorFactor;

    // Initialize textures (if valid AssetInfo is provided)
    if (materialData.baseColorTexture) {
        baseColorTexture = assetHandleManager->getOrLoadResource<TextureDescriptor>(materialData.baseColorTexture->id);
        descriptorBindingFlags |= vks::DescriptorBindingFlags::ImageBaseColor;
    }
    if (materialData.metallicRoughnessTexture) {
        metallicRoughnessTexture = assetHandleManager->getOrLoadResource<TextureDescriptor>(materialData.metallicRoughnessTexture->id);
    }
    if (materialData.normalTexture) {
        normalTexture = assetHandleManager->getOrLoadResource<TextureDescriptor>(materialData.normalTexture->id);
        descriptorBindingFlags |= vks::DescriptorBindingFlags::ImageNormalMap;
    }
    if (materialData.occlusionTexture) {
        occlusionTexture = assetHandleManager->getOrLoadResource<TextureDescriptor>(materialData.occlusionTexture->id);
    }
    if (materialData.emissiveTexture) {
        emissiveTexture = assetHandleManager->getOrLoadResource<TextureDescriptor>(materialData.emissiveTexture->id);
    }
    if (materialData.specularGlossinessTexture) {
        metallicRoughnessTexture = assetHandleManager->getOrLoadResource<TextureDescriptor>(materialData.specularGlossinessTexture->id);
    }

    // Setup descriptors - ALWAYS create descriptor set, even if no textures
    setUpDescriptorSet(assetHandleManager->pbrMaterialLayout, assetHandleManager->pbrMaterialPool, assetHandleManager->defaultImageInfo, assetHandleManager->cubeImageInfo);
}
vks::MaterialDescriptor::~MaterialDescriptor() {
    delete baseColorTexture;
    delete metallicRoughnessTexture;
    delete normalTexture;
    delete occlusionTexture;
    delete emissiveTexture;
}

void vks::MaterialDescriptor::setUpDescriptorSet(VkDescriptorSetLayout materialLayout, VkDescriptorPool materialDescriptorPool, VkDescriptorImageInfo defaultImageInfo, VkDescriptorImageInfo defaultCubeImageInfo) {
    VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
    descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocInfo.descriptorPool = materialDescriptorPool;
    descriptorSetAllocInfo.pSetLayouts = &materialLayout;
    descriptorSetAllocInfo.descriptorSetCount = 1;
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, &descriptorSet));

    std::vector<VkWriteDescriptorSet> writeDescriptorSets{};

    // Binding 0: Sampler
    VkDescriptorImageInfo samplerInfo{};
    samplerInfo.imageView = VK_NULL_HANDLE;

    // Use cube sampler if we have a cube texture in base color
    if (baseColorTexture && baseColorTexture->descriptor.sampler == assetHandleManager->cubeSampler) {
        samplerInfo.sampler = assetHandleManager->cubeSampler;
    } else {
        samplerInfo.sampler = assetHandleManager->defaultSampler;
    }

    VkWriteDescriptorSet samplerWrite{};
    samplerWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    samplerWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    samplerWrite.descriptorCount = 1;
    samplerWrite.dstSet = descriptorSet;
    samplerWrite.dstBinding = 0;
    samplerWrite.pImageInfo = &samplerInfo;
    writeDescriptorSets.push_back(samplerWrite);

    // Binding 1: Base Color Image (Sampled Image)
    VkDescriptorImageInfo baseColorImageInfo{};
    baseColorImageInfo.sampler = VK_NULL_HANDLE;  // No sampler for SAMPLED_IMAGE!
    baseColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    if (baseColorTexture) {
        baseColorImageInfo.imageView = baseColorTexture->descriptor.imageView;
    } else {
        // If it's a skybox material layout, use cube default
        if (materialLayout == assetHandleManager->skyboxMaterialLayout) {
             baseColorImageInfo.imageView = defaultCubeImageInfo.imageView;
        } else {
             baseColorImageInfo.imageView = defaultImageInfo.imageView;
        }
    }

    VkWriteDescriptorSet baseColorWrite{};
    baseColorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    baseColorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    baseColorWrite.descriptorCount = 1;
    baseColorWrite.dstSet = descriptorSet;
    baseColorWrite.dstBinding = 1;
    baseColorWrite.pImageInfo = &baseColorImageInfo;
    writeDescriptorSets.push_back(baseColorWrite);

    // Binding 2: Normal Map Image
    VkDescriptorImageInfo normalMapImageInfo{};
    normalMapImageInfo.sampler = VK_NULL_HANDLE;  // No sampler for SAMPLED_IMAGE!
    normalMapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    if (normalTexture) {
        normalMapImageInfo.imageView = normalTexture->descriptor.imageView;
    } else {
        normalMapImageInfo.imageView = defaultImageInfo.imageView;
    }

    VkWriteDescriptorSet normalMapWrite{};
    normalMapWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    normalMapWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    normalMapWrite.descriptorCount = 1;
    normalMapWrite.dstSet = descriptorSet;
    normalMapWrite.dstBinding = 2;
    normalMapWrite.pImageInfo = &normalMapImageInfo;
    writeDescriptorSets.push_back(normalMapWrite);

    if (!writeDescriptorSets.empty()) {
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()),
                              writeDescriptorSets.data(), 0, nullptr);
    }
}