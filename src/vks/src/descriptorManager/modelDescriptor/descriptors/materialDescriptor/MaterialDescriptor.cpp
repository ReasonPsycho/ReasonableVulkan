//
// Created by redkc on 10/08/2025.
//

#include "MaterialDescriptor.h"

#include "Asset.hpp"
#include "../../../DescriptorManager.h"
#
// Static member definition


vks::MaterialDescriptor::MaterialDescriptor(DescriptorManager* assetHandleManager, am::MaterialData& materialData,VulkanContext& vulkanContext) : IVulkanDescriptor(vulkanContext)
{
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
        metallicRoughnessTexture = assetHandleManager->getOrLoadResource<TextureDescriptor>(materialData.specularGlossinessTexture->id); // TODO this smells xd
    }
    
    // Setup descriptors if we have textures
    if (baseColorTexture) {
        setUpDescriptorSet(assetHandleManager->materialLayout,assetHandleManager->materialPool);
    }
}

vks::MaterialDescriptor::~MaterialDescriptor() {
    delete baseColorTexture;
    delete metallicRoughnessTexture;
    delete normalTexture;
    delete occlusionTexture;
    delete emissiveTexture;
}



void vks::MaterialDescriptor::setUpDescriptorSet(VkDescriptorSetLayout materialLayout,VkDescriptorPool materialDescriptorPool) {
    VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
    descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocInfo.descriptorPool = materialDescriptorPool;
    descriptorSetAllocInfo.pSetLayouts = &materialLayout;
    descriptorSetAllocInfo.descriptorSetCount = 1;
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, &descriptorSet));

    std::vector<VkWriteDescriptorSet> writeDescriptorSets{};

    // Only add descriptor writes for textures that are actually present
    if (baseColorTexture) {
        VkWriteDescriptorSet baseColorWrite{};
        baseColorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        baseColorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        baseColorWrite.descriptorCount = 1;
        baseColorWrite.dstSet = descriptorSet;
        baseColorWrite.dstBinding = 0;
        baseColorWrite.pImageInfo = &baseColorTexture->descriptor;
        writeDescriptorSets.push_back(baseColorWrite);
    }

    /*if (normalTexture) { For now we don't care
        VkWriteDescriptorSet normalMapWrite{};
        normalMapWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        normalMapWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        normalMapWrite.descriptorCount = 1;
        normalMapWrite.dstSet = descriptorSet;
        normalMapWrite.dstBinding = 1;
        normalMapWrite.pImageInfo = &normalTexture->descriptor;
        writeDescriptorSets.push_back(normalMapWrite);
    }*/

    if (!writeDescriptorSets.empty()) {
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()),
                              writeDescriptorSets.data(), 0, nullptr);
    }
}