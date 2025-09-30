#include "ModelDescriptor.h"
#include "../DescriptorManager.h"


void vks::ModelDescriptor::loadNode(DescriptorManager* assetHandleManager,NodeDescriptorStruct* parent, const am::Node& node,
                                vks::ModelDescriptor& model) {
	vks::NodeDescriptorStruct *newNode = new vks::NodeDescriptorStruct();
	newNode->parent = parent;
	newNode->name = node.mName;
	newNode->matrix = node.mTransformation;

	// Node with children
		for (auto i = 0; i < node.mChildren.size(); i++) {
			loadNode(assetHandleManager,newNode, node.mChildren[i], model);
		}

    // Node contains mesh data
        for (auto i = 0; i < node.meshes.size(); i++) {
                MeshDescriptor* meshHandle =  assetHandleManager->getOrLoadResource<MeshDescriptor>(node.meshes[i]->id);
                model.meshes.push_back(meshHandle);

        	VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
        	descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        	descriptorSetAllocInfo.descriptorPool = descriptorPool;
        	descriptorSetAllocInfo.pSetLayouts = &assetHandleManager->meshUniformLayout;
        	descriptorSetAllocInfo.descriptorSetCount = 1;
        	VK_CHECK_RESULT(
				vkAllocateDescriptorSets(device.logicalDevice, &descriptorSetAllocInfo, &meshHandle->uniformBuffer.descriptorSet));

        	VkWriteDescriptorSet writeDescriptorSet{};
        	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        	writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        	writeDescriptorSet.descriptorCount = 1;
        	writeDescriptorSet.dstSet = meshHandle->uniformBuffer.descriptorSet;
        	writeDescriptorSet.dstBinding = 0;
        	writeDescriptorSet.pBufferInfo = &meshHandle->uniformBuffer.descriptor;

        	vkUpdateDescriptorSets(device.logicalDevice, 1, &writeDescriptorSet, 0, nullptr);

        }

    if (parent) {
		parent->children.push_back(newNode);
	} else {
		nodes.push_back(newNode);
	}
}

vks::ModelDescriptor::ModelDescriptor(DescriptorManager* assetHandleManager,am::ModelData modelData,vks::base::VulkanDevice device, VkQueue transferQueue) : IVulkanDescriptor(device,transferQueue)
{
    loadNode(assetHandleManager,nullptr, modelData.rootNode, *this);
}

vks::ModelDescriptor::~ModelDescriptor()
{

}
