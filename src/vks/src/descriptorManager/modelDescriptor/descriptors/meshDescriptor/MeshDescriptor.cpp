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

vks::MeshDescriptor::MeshDescriptor(DescriptorManager* assetHandleManager,am::MeshData& meshData, glm::mat4 matrix, vks::base::VulkanDevice* device, VkQueue* copyQueue) : IVulkanDescriptor(device,copyQueue),
    firstIndex(0), indexCount(0), firstVertex(0), vertexCount(0)
{
    this->device = device;
    this->uniformBlock.matrix = matrix;
    this->material = assetHandleManager->getOrLoadResource<vks::MaterialDescriptor>(meshData.material->id);

    // Create vertex buffer
    VkDeviceSize vertexBufferSize = sizeof(am::VertexAsset) * meshData.vertices.size();
    vks::base::Buffer stagingBuffer;

    device->createBuffer(
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        vertexBufferSize,
        &stagingBuffer.buffer,
        &stagingBuffer.memory,
        meshData.vertices.data());

    device->createBuffer(
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        vertexBufferSize,
        &vertices.buffer.buffer,
        &vertices.buffer.memory,
        nullptr);

    device->copyBuffer(&stagingBuffer, &vertices.buffer, *copyQueue);
    vertices.count = meshData.vertices.size();

    vkDestroyBuffer(device->logicalDevice, stagingBuffer.buffer, nullptr);
    vkFreeMemory(device->logicalDevice, stagingBuffer.memory, nullptr);

    // Create index buffer
    VkDeviceSize indexBufferSize = sizeof(uint32_t) * meshData.indices.size();

    device->createBuffer(
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        indexBufferSize,
        &stagingBuffer.buffer,
        &stagingBuffer.memory,
        meshData.indices.data());

    device->createBuffer(
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        indexBufferSize,
        &indices.buffer.buffer,
        &indices.buffer.memory,
        nullptr);

    device->copyBuffer(&stagingBuffer, &indices.buffer, *copyQueue);
    indices.count = meshData.indices.size();

    vkDestroyBuffer(device->logicalDevice, stagingBuffer.buffer, nullptr);
    vkFreeMemory(device->logicalDevice, stagingBuffer.memory, nullptr);

    // Setup uniform buffer
    VK_CHECK_RESULT(device->createBuffer(
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        sizeof(uniformBlock),
        &uniformBuffer.buffer.buffer,
        &uniformBuffer.buffer.memory,
        &uniformBlock));
}

vks::MeshDescriptor::~MeshDescriptor() {
    vkDestroyBuffer(device->logicalDevice, uniformBuffer.buffer.buffer, nullptr);
    vkFreeMemory(device->logicalDevice, uniformBuffer.buffer.memory, nullptr);

    vkDestroyBuffer(device->logicalDevice, vertices.buffer.buffer, nullptr);
    vkFreeMemory(device->logicalDevice, vertices.buffer.memory, nullptr);

    vkDestroyBuffer(device->logicalDevice, uniformBuffer.buffer.buffer, nullptr);
    vkFreeMemory(device->logicalDevice, uniformBuffer.buffer.memory, nullptr);


    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device->logicalDevice, descriptorPool, nullptr);
    }
}

VkVertexInputBindingDescription vks::MeshDescriptor::inputBindingDescription(uint32_t binding) {
    return VkVertexInputBindingDescription({binding, sizeof(am::VertexAsset), VK_VERTEX_INPUT_RATE_VERTEX});
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


void vks::MeshDescriptor::createDescriptorPool() {
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


void vks::MeshDescriptor::createDescriptorSet(VkDescriptorSetLayout meshUniformLayout) {
    VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
    descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocInfo.descriptorPool = descriptorPool;
    descriptorSetAllocInfo.pSetLayouts = &meshUniformLayout;
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

void vks::MeshDescriptor::setupDescriptors(VkDescriptorSetLayout meshUniformLayout) {
    createDescriptorPool();
    createDescriptorSet(meshUniformLayout);
}