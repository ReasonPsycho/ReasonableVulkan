//
// Created by redkc on 10/08/2025.
//

#include "MeshHandle.h"

#include "Asset.hpp"
#include "../../../base/VulkanDevice.hpp"
#include "../../../../base/VulkanInitializers.hpp"

// Static member definition
VkDescriptorSetLayout vks::MeshHandle::descriptorSetLayoutUbo = VK_NULL_HANDLE;

vks::MeshHandle::MeshHandle(base::VulkanDevice *device, am::MeshData& meshData, glm::mat4 matrix, VkQueue copyQueue) : IVulkanHandle(device,copyQueue),
    material(*new MaterialHandle(device, *meshData.material->getAsset()->getAssetDataAs<am::MaterialData>(), copyQueue)), 
    firstIndex(0), indexCount(0), firstVertex(0), vertexCount(0)
{
    // ... existing buffer creation code ...
    
    this->device = device;
    this->uniformBlock.matrix = matrix;
    
    VK_CHECK_RESULT(device->createBuffer(
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        sizeof(uniformBlock),
        &uniformBuffer.buffer,
        &uniformBuffer.memory,
        &uniformBlock));
    VK_CHECK_RESULT(
        vkMapMemory(device->logicalDevice, uniformBuffer.memory, 0, sizeof(uniformBlock), 0, &uniformBuffer.mapped));
    uniformBuffer.descriptor = {uniformBuffer.buffer, 0, sizeof(uniformBlock)};
    
    // Setup descriptors for this mesh
    setupDescriptors();
}

vks::MeshHandle::~MeshHandle() {
    vkDestroyBuffer(device->logicalDevice, uniformBuffer.buffer, nullptr);
    vkFreeMemory(device->logicalDevice, uniformBuffer.memory, nullptr);
    
    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device->logicalDevice, descriptorPool, nullptr);
    }
    
    delete &material;
}

void vks::MeshHandle::createDescriptorPool() {
    std::vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
    };
    
    VkDescriptorPoolCreateInfo descriptorPoolCI{};
    descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    descriptorPoolCI.pPoolSizes = poolSizes.data();
    descriptorPoolCI.maxSets = 1;
    VK_CHECK_RESULT(vkCreateDescriptorPool(device->logicalDevice, &descriptorPoolCI, nullptr, &descriptorPool));
}

void vks::MeshHandle::createDescriptorSetLayout() {
    if (descriptorSetLayoutUbo == VK_NULL_HANDLE) {
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
            vks::base::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                                VK_SHADER_STAGE_VERTEX_BIT, 0),
        };
        VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{};
        descriptorLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
        descriptorLayoutCI.pBindings = setLayoutBindings.data();
        VK_CHECK_RESULT(
            vkCreateDescriptorSetLayout(device->logicalDevice, &descriptorLayoutCI, nullptr, &descriptorSetLayoutUbo));
    }
}

void vks::MeshHandle::createDescriptorSet() {
    VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
    descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocInfo.descriptorPool = descriptorPool;
    descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayoutUbo;
    descriptorSetAllocInfo.descriptorSetCount = 1;
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device->logicalDevice, &descriptorSetAllocInfo, &uniformBuffer.descriptorSet));

    VkWriteDescriptorSet writeDescriptorSet{};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.dstSet = uniformBuffer.descriptorSet;
    writeDescriptorSet.dstBinding = 0;
    writeDescriptorSet.pBufferInfo = &uniformBuffer.descriptor;

    vkUpdateDescriptorSets(device->logicalDevice, 1, &writeDescriptorSet, 0, nullptr);
}

void vks::MeshHandle::setupDescriptors() {
    createDescriptorSetLayout();
    createDescriptorPool();
    createDescriptorSet();
}
