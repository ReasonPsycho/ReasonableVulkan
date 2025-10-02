//
// Created by redkc on 10/08/2025.
//

#include "MeshDescriptor.h"
#include "Asset.hpp"
#include "../../../DescriptorManager.h"

namespace vks {
    VkVertexInputBindingDescription MeshDescriptor::vertexInputBindingDescription{};
    std::vector<VkVertexInputAttributeDescription> MeshDescriptor::vertexInputAttributeDescriptions{};
    VkPipelineVertexInputStateCreateInfo MeshDescriptor::pipelineVertexInputStateCreateInfo{};
}

vks::MeshDescriptor::MeshDescriptor(DescriptorManager* assetHandleManager, am::MeshData& meshData, 
    glm::mat4 matrix, VulkanContext& vulkanContext) 
    : IVulkanDescriptor(vulkanContext),
    firstIndex(0), indexCount(0), firstVertex(0), vertexCount(0)
{
    this->uniformBlock.matrix = matrix;
    this->material = assetHandleManager->getOrLoadResource<vks::MaterialDescriptor>(meshData.material->id);

    // Create vertex buffer
    VkDeviceSize vertexBufferSize = sizeof(am::VertexAsset) * meshData.vertices.size();
    
    // Create staging buffer for vertices
    VkBuffer vertexStagingBuffer;
    VkDeviceMemory vertexStagingMemory;
    
    vulkanContext.createBuffer(
        vertexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        vertexStagingBuffer,
        vertexStagingMemory);

    // Copy vertex data to staging buffer
    void* data;
    vkMapMemory(vulkanContext.getDevice(), vertexStagingMemory, 0, vertexBufferSize, 0, &data);
    memcpy(data, meshData.vertices.data(), vertexBufferSize);
    vkUnmapMemory(vulkanContext.getDevice(), vertexStagingMemory);

    // Create device local vertex buffer
    vulkanContext.createBuffer(
        vertexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        vertices.buffer.buffer,
        vertices.buffer.memory);

    // Copy from staging to device local buffer
    vulkanContext.copyBuffer(
        vertexStagingBuffer,
        vertices.buffer.buffer,
        vertexBufferSize,
        QueueType::Transfer);

    vertices.count = meshData.vertices.size();

    // Clean up vertex staging buffer
    vkDestroyBuffer(vulkanContext.getDevice(), vertexStagingBuffer, nullptr);
    vkFreeMemory(vulkanContext.getDevice(), vertexStagingMemory, nullptr);

    // Create index buffer
    VkDeviceSize indexBufferSize = sizeof(uint32_t) * meshData.indices.size();
    
    // Create staging buffer for indices
    VkBuffer indexStagingBuffer;
    VkDeviceMemory indexStagingMemory;
    
    vulkanContext.createBuffer(
        indexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        indexStagingBuffer,
        indexStagingMemory);

    // Copy index data to staging buffer
    vkMapMemory(vulkanContext.getDevice(), indexStagingMemory, 0, indexBufferSize, 0, &data);
    memcpy(data, meshData.indices.data(), indexBufferSize);
    vkUnmapMemory(vulkanContext.getDevice(), indexStagingMemory);

    // Create device local index buffer
    vulkanContext.createBuffer(
        indexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        indices.buffer.buffer,
        indices.buffer.memory);

    // Copy from staging to device local buffer
    vulkanContext.copyBuffer(
        indexStagingBuffer,
        indices.buffer.buffer,
        indexBufferSize,
        QueueType::Transfer);

    indices.count = meshData.indices.size();

    // Clean up index staging buffer
    vkDestroyBuffer(vulkanContext.getDevice(), indexStagingBuffer, nullptr);
    vkFreeMemory(vulkanContext.getDevice(), indexStagingMemory, nullptr);

    // Create uniform buffer
    vulkanContext.createBuffer(
        sizeof(UniformBlock),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        uniformBuffer.buffer.buffer,
        uniformBuffer.buffer.memory);

    // Setup uniform buffer descriptor
    uniformBuffer.descriptor.buffer = uniformBuffer.buffer.buffer;
    uniformBuffer.descriptor.offset = 0;
    uniformBuffer.descriptor.range = sizeof(UniformBlock);

    // Map uniform buffer
    VK_CHECK_RESULT(vkMapMemory(vulkanContext.getDevice(), uniformBuffer.buffer.memory, 
        0, sizeof(UniformBlock), 0, &uniformBuffer.mapped));
    memcpy(uniformBuffer.mapped, &uniformBlock, sizeof(UniformBlock));
}

vks::MeshDescriptor::~MeshDescriptor() {

    if (uniformBuffer.buffer.buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, uniformBuffer.buffer.buffer, nullptr);
        vkFreeMemory(device, uniformBuffer.buffer.memory, nullptr);
    }

    if (vertices.buffer.buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, vertices.buffer.buffer, nullptr);
        vkFreeMemory(device, vertices.buffer.memory, nullptr);
    }

    if (indices.buffer.buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, indices.buffer.buffer, nullptr);
        vkFreeMemory(device, indices.buffer.memory, nullptr);
    }
}

void vks::MeshDescriptor::setUpDescriptorSet(VkDescriptorSetLayout meshUniformLayout,VkDescriptorPool meshDescriptorPool) {
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = meshDescriptorPool;
    allocInfo.pSetLayouts = &meshUniformLayout;
    allocInfo.descriptorSetCount = 1;
    
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &uniformBuffer.descriptorSet));

    VkWriteDescriptorSet writeDescriptorSet{};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.dstSet = uniformBuffer.descriptorSet;
    writeDescriptorSet.dstBinding = 0;
    writeDescriptorSet.pBufferInfo = &uniformBuffer.descriptor;

    vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
}


VkVertexInputAttributeDescription vks::MeshDescriptor::inputAttributeDescription(
    uint32_t binding, uint32_t location, VertexComponent component) {
    switch (component) {
        case VertexComponent::Position:
            return VkVertexInputAttributeDescription({
                location, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(am::VertexAsset, Position)
            });
        case VertexComponent::Normal:
            return VkVertexInputAttributeDescription({
                location, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(am::VertexAsset, Normal)
            });
        case VertexComponent::UV:
            return VkVertexInputAttributeDescription({
                location, binding, VK_FORMAT_R32G32_SFLOAT, offsetof(am::VertexAsset, TexCoords)
            });
        case VertexComponent::Color:
            return VkVertexInputAttributeDescription({
                location, binding, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(am::VertexAsset, Color)
            });
        case VertexComponent::Tangent:
            return VkVertexInputAttributeDescription({
                location, binding, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(am::VertexAsset, Tangent)
            });
        case VertexComponent::Bitangent:
            return VkVertexInputAttributeDescription({
                location, binding, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(am::VertexAsset, Bitangent)
            });
        default:
            return VkVertexInputAttributeDescription({});
    }
}

std::vector<VkVertexInputAttributeDescription> vks::MeshDescriptor::inputAttributeDescriptions(
    uint32_t binding, const std::vector<VertexComponent> components) {
    std::vector<VkVertexInputAttributeDescription> result;
    uint32_t location = 0;
    for (VertexComponent component: components) {
        result.push_back(MeshDescriptor::inputAttributeDescription(binding, location, component));
        location++;
    }
    return result;
}


VkVertexInputBindingDescription vks::MeshDescriptor::inputBindingDescription(uint32_t binding) {
    return VkVertexInputBindingDescription({binding, sizeof(am::VertexAsset), VK_VERTEX_INPUT_RATE_VERTEX});
}

VkPipelineVertexInputStateCreateInfo* vks::MeshDescriptor::getPipelineVertexInputState(
    const std::vector<VertexComponent> components) {
    vertexInputBindingDescription = inputBindingDescription(0);
    vertexInputAttributeDescriptions = inputAttributeDescriptions(0, components);
    pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
    pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexInputBindingDescription;
    pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributeDescriptions.size());
    pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data();
    return &pipelineVertexInputStateCreateInfo;
}
