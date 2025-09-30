//
// Created by redkc on 10/08/2025.
//

#include "MaterialDescriptor.h"

#include "Asset.hpp"
#include "../../../../base/VulkanInitializers.hpp"

// Static member definition


vks::MaterialDescriptor::MaterialDescriptor( am::MaterialData& materialData,VkDescriptorSetLayout materialLayout, vks::base::VulkanDevice& device, VkQueue copyQueue) : IVulkanDescriptor(device,copyQueue)
{
    // Initialize numeric fields
    alphaCutoff = materialData.alphaCutoff;
    metallicFactor = materialData.metallicFactor;
    roughnessFactor = materialData.roughnessFactor;
    baseColorFactor = materialData.baseColorFactor;

    // Initialize textures (if valid AssetInfo is provided)
    if (materialData.baseColorTexture) {
        baseColorTexture = new TextureDescriptor(*materialData.baseColorTexture->getAsset()->getAssetDataAs<am::TextureData>(), device, copyQueue);
        descriptorBindingFlags |= vks::DescriptorBindingFlags::ImageBaseColor;
    }
    if (materialData.metallicRoughnessTexture) {
        metallicRoughnessTexture = new TextureDescriptor(*materialData.metallicRoughnessTexture->getAsset()->getAssetDataAs<am::TextureData>(), device, copyQueue);
    }
    if (materialData.normalTexture) {
        normalTexture = new TextureDescriptor(*materialData.normalTexture->getAsset()->getAssetDataAs<am::TextureData>(), device, copyQueue);
        descriptorBindingFlags |= vks::DescriptorBindingFlags::ImageNormalMap;
    }
    if (materialData.occlusionTexture) {
        occlusionTexture = new TextureDescriptor(*materialData.occlusionTexture->getAsset()->getAssetDataAs<am::TextureData>(), device, copyQueue);
    }
    if (materialData.emissiveTexture) {
        emissiveTexture = new TextureDescriptor(*materialData.emissiveTexture->getAsset()->getAssetDataAs<am::TextureData>(), device, copyQueue);
    }
    if (materialData.specularGlossinessTexture) {
        metallicRoughnessTexture = new TextureDescriptor(*materialData.specularGlossinessTexture->getAsset()->getAssetDataAs<am::TextureData>(), device, copyQueue);
    }
    
    // Setup descriptors if we have textures
    if (baseColorTexture || normalTexture) {
        setupDescriptors(materialLayout);
    }
}

vks::MaterialDescriptor::~MaterialDescriptor() {
    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    }
    
    delete baseColorTexture;
    delete metallicRoughnessTexture;
    delete normalTexture;
    delete occlusionTexture;
    delete emissiveTexture;
}

void vks::MaterialDescriptor::createDescriptorPool() {
    std::vector<VkDescriptorPoolSize> poolSizes;
    uint32_t imageCount = 0;
    
    if (baseColorTexture && descriptorBindingFlags & DescriptorBindingFlags::ImageBaseColor) {
        imageCount++;
    }
    if (normalTexture && descriptorBindingFlags & DescriptorBindingFlags::ImageNormalMap) {
        imageCount++;
    }
    
    if (imageCount > 0) {
        poolSizes.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageCount});
    }
    
    if (poolSizes.empty()) {
        return; // No descriptors needed
    }
    
    VkDescriptorPoolCreateInfo descriptorPoolCI{};
    descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    descriptorPoolCI.pPoolSizes = poolSizes.data();
    descriptorPoolCI.maxSets = 1;
    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr, &descriptorPool));
}


void vks::MaterialDescriptor::createDescriptorSet(VkDescriptorSetLayout materialLayout) {
    if (descriptorPool == VK_NULL_HANDLE) {
        return; // No pool means no descriptors needed
    }
    
    VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
    descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocInfo.descriptorPool = descriptorPool;
    descriptorSetAllocInfo.pSetLayouts = &materialLayout;
    descriptorSetAllocInfo.descriptorSetCount = 1;
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, &descriptorSet));
    
    std::vector<VkWriteDescriptorSet> writeDescriptorSets{};
    if (baseColorTexture && descriptorBindingFlags & DescriptorBindingFlags::ImageBaseColor) {
        VkWriteDescriptorSet writeDescriptorSet{};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.dstSet = descriptorSet;
        writeDescriptorSet.dstBinding = static_cast<uint32_t>(writeDescriptorSets.size());
        writeDescriptorSet.pImageInfo = &baseColorTexture->descriptor;
        writeDescriptorSets.push_back(writeDescriptorSet);
    }
    if (normalTexture && descriptorBindingFlags & DescriptorBindingFlags::ImageNormalMap) {
        VkWriteDescriptorSet writeDescriptorSet{};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.dstSet = descriptorSet;
        writeDescriptorSet.dstBinding = static_cast<uint32_t>(writeDescriptorSets.size());
        writeDescriptorSet.pImageInfo = &normalTexture->descriptor;
        writeDescriptorSets.push_back(writeDescriptorSet);
    }
    
    if (!writeDescriptorSets.empty()) {
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()),
                               writeDescriptorSets.data(), 0, nullptr);
    }
}

void vks::MaterialDescriptor::setupDescriptors(VkDescriptorSetLayout materialLayout) {
    createDescriptorPool();
    createDescriptorSet(materialLayout);
}
