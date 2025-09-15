//
// Created by redkc on 10/08/2025.
//

#include "MaterialHandle.h"

#include "Asset.hpp"
#include "../../../base/VulkanDevice.hpp"
#include "../../../../base/VulkanInitializers.hpp"

// Static member definition
VkDescriptorSetLayout vks::MaterialHandle::descriptorSetLayoutImage = VK_NULL_HANDLE;

vks::MaterialHandle::MaterialHandle(base::VulkanDevice* device, am::MaterialData& materialData, VkQueue copyQueue) : IVulkanHandle(device,copyQueue)
{
    // Initialize numeric fields
    alphaCutoff = materialData.alphaCutoff;
    metallicFactor = materialData.metallicFactor;
    roughnessFactor = materialData.roughnessFactor;
    baseColorFactor = materialData.baseColorFactor;

    // Initialize textures (if valid AssetInfo is provided)
    if (materialData.baseColorTexture) {
        baseColorTexture = new TextureHandle(*materialData.baseColorTexture->getAsset()->getAssetDataAs<am::TextureData>(), device, copyQueue);
        descriptorBindingFlags |= DescriptorBindingFlags::ImageBaseColor;
    }
    if (materialData.metallicRoughnessTexture) {
        metallicRoughnessTexture = new TextureHandle(*materialData.metallicRoughnessTexture->getAsset()->getAssetDataAs<am::TextureData>(), device, copyQueue);
    }
    if (materialData.normalTexture) {
        normalTexture = new TextureHandle(*materialData.normalTexture->getAsset()->getAssetDataAs<am::TextureData>(), device, copyQueue);
        descriptorBindingFlags |= DescriptorBindingFlags::ImageNormalMap;
    }
    if (materialData.occlusionTexture) {
        occlusionTexture = new TextureHandle(*materialData.occlusionTexture->getAsset()->getAssetDataAs<am::TextureData>(), device, copyQueue);
    }
    if (materialData.emissiveTexture) {
        emissiveTexture = new TextureHandle(*materialData.emissiveTexture->getAsset()->getAssetDataAs<am::TextureData>(), device, copyQueue);
    }
    if (materialData.specularGlossinessTexture) {
        metallicRoughnessTexture = new TextureHandle(*materialData.specularGlossinessTexture->getAsset()->getAssetDataAs<am::TextureData>(), device, copyQueue);
    }
    
    // Setup descriptors if we have textures
    if (baseColorTexture || normalTexture) {
        setupDescriptors();
    }
}

vks::MaterialHandle::~MaterialHandle() {
    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device->logicalDevice, descriptorPool, nullptr);
    }
    
    delete baseColorTexture;
    delete metallicRoughnessTexture;
    delete normalTexture;
    delete occlusionTexture;
    delete emissiveTexture;
}

void vks::MaterialHandle::createDescriptorPool() {
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
    VK_CHECK_RESULT(vkCreateDescriptorPool(device->logicalDevice, &descriptorPoolCI, nullptr, &descriptorPool));
}

void vks::MaterialHandle::createDescriptorSetLayout() {
    if (descriptorSetLayoutImage == VK_NULL_HANDLE) {
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
        if (descriptorBindingFlags & DescriptorBindingFlags::ImageBaseColor) {
            setLayoutBindings.push_back(vks::base::initializers::descriptorSetLayoutBinding(
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT,
                static_cast<uint32_t>(setLayoutBindings.size())));
        }
        if (descriptorBindingFlags & DescriptorBindingFlags::ImageNormalMap) {
            setLayoutBindings.push_back(vks::base::initializers::descriptorSetLayoutBinding(
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT,
                static_cast<uint32_t>(setLayoutBindings.size())));
        }
        
        if (!setLayoutBindings.empty()) {
            VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{};
            descriptorLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
            descriptorLayoutCI.pBindings = setLayoutBindings.data();
            VK_CHECK_RESULT(
                vkCreateDescriptorSetLayout(device->logicalDevice, &descriptorLayoutCI, nullptr, &descriptorSetLayoutImage));
        }
    }
}

void vks::MaterialHandle::createDescriptorSet() {
    if (descriptorPool == VK_NULL_HANDLE) {
        return; // No pool means no descriptors needed
    }
    
    VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
    descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocInfo.descriptorPool = descriptorPool;
    descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayoutImage;
    descriptorSetAllocInfo.descriptorSetCount = 1;
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device->logicalDevice, &descriptorSetAllocInfo, &descriptorSet));
    
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
        vkUpdateDescriptorSets(device->logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()),
                               writeDescriptorSets.data(), 0, nullptr);
    }
}

void vks::MaterialHandle::setupDescriptors() {
    createDescriptorSetLayout();
    createDescriptorPool();
    createDescriptorSet();
}
