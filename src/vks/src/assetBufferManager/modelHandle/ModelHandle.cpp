#include "ModelHandle.h"

#include "../AssetHandleManager.h"
#include "handles/vertexHandle/VertexHandle.h"



void vks::ModelHandle::loadNode(AssetHandleManager assetHandleManager,NodeHandle* parent, const am::Node& node,
					  const vks::ModelHandle& model, std::vector<uint32_t>& indexBuffer,
					  std::vector<am::VertexHandle>& vertexBuffer) {
	vks::NodeHandle *newNode = new vks::NodeHandle{};
	newNode->parent = parent;
	newNode->name = node.mName;
	newNode->matrix = node.mTransformation;

	// Node with children
		for (auto i = 0; i < node.mChildren.size(); i++) {
			loadNode(assetHandleManager,newNode, node.mChildren[i], model, model, indexBuffer, vertexBuffer);
		}

    // Node contains mesh data
        for (auto i = 0; i < node.meshes.size(); i++) {
                MeshHandle* meshHandle = assetHandleManager.getOrLoadResource(node.meshes[i]->id);
                model.meshes.push_back(meshHandle);

        	VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
        	descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        	descriptorSetAllocInfo.descriptorPool = descriptorPool;
        	descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;
        	descriptorSetAllocInfo.descriptorSetCount = 1;
        	VK_CHECK_RESULT(
				vkAllocateDescriptorSets(device->logicalDevice, &descriptorSetAllocInfo, &mesh->uniformBuffer.descriptorSet));

        	VkWriteDescriptorSet writeDescriptorSet{};
        	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        	writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        	writeDescriptorSet.descriptorCount = 1;
        	writeDescriptorSet.dstSet = mesh->uniformBuffer.descriptorSet;
        	writeDescriptorSet.dstBinding = 0;
        	writeDescriptorSet.pBufferInfo = &mesh->uniformBuffer.descriptor;

        	vkUpdateDescriptorSets(device->logicalDevice, 1, &writeDescriptorSet, 0, nullptr);

        }

    if (parent) {
		parent->children.push_back(newNode);
	} else {
		nodes.push_back(newNode);
	}
}

vks::ModelHandle::ModelHandle(am::ModelData modelData, vks::base::VulkanDevice* device, VkQueue transferQueue) : IVulkanHandle(device,copyQueue)
{
    std::vector<uint32_t> indexBuffer;
    std::vector<am::VertexHandle> vertexBuffer;

    loadNode(modelData.rootNode, nullptr, modelData.rootNode, modelData, indexBuffer, vertexBuffer);

    // Pre-Calculations for requested features
    size_t vertexBufferSize = vertexBuffer.size() * sizeof(VertexHandle);
    size_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);

    assert((vertexBufferSize > 0) && (indexBufferSize > 0));

    struct StagingBuffer
    {
        VkBuffer buffer;
        VkDeviceMemory memory;
    } vertexStaging, indexStaging;

    // Note: Descriptor management is now handled by individual MeshHandle and MaterialHandle instances
    // Each mesh and material manages its own descriptors independently
}

void Model::prepareNodeDescriptor(Node* node, VkDescriptorSetLayout descriptorSetLayout)
{
    if (node->mesh)
    {
        VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
        descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocInfo.descriptorPool = descriptorPool;
        descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;
        descriptorSetAllocInfo.descriptorSetCount = 1;
        VK_CHECK_RESULT(
            vkAllocateDescriptorSets(device->logicalDevice, &descriptorSetAllocInfo, &node->mesh->uniformBuffer.
                descriptorSet));

        VkWriteDescriptorSet writeDescriptorSet{};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.dstSet = node->mesh->uniformBuffer.descriptorSet;
        writeDescriptorSet.dstBinding = 0;
        writeDescriptorSet.pBufferInfo = &node->mesh->uniformBuffer.descriptor;

        vkUpdateDescriptorSets(device->logicalDevice, 1, &writeDescriptorSet, 0, nullptr);
    }
    for (auto& child : node->children)
    {
        prepareNodeDescriptor(child, descriptorSetLayout);
    }
}
