#include "ModelDescriptor.h"
#include "../DescriptorManager.h"


void vks::ModelDescriptor::loadNode(DescriptorManager* assetHandleManager,NodeDescriptorStruct* parent, const am::Node& node,
                                vks::ModelDescriptor& model,VulkanContext& vulkanContext) {
	vks::NodeDescriptorStruct *newNode = new vks::NodeDescriptorStruct();
	newNode->parent = parent;
	newNode->name = node.mName;
	newNode->matrix = node.mTransformation;

	// Node with children
		for (auto i = 0; i < node.mChildren.size(); i++) {
			loadNode(assetHandleManager,newNode, node.mChildren[i], model,vulkanContext);
		}

    // Node contains mesh data
        for (auto i = 0; i < node.meshes.size(); i++) {
            if (node.meshes[i] && !node.meshes[i]->id.is_nil() && assetHandleManager) {
                MeshDescriptor* meshHandle =  assetHandleManager->getOrLoadResource<MeshDescriptor>(node.meshes[i]->id);
                if (meshHandle) {
                    model.meshes.push_back(meshHandle);
                    newNode->meshes.push_back(meshHandle);
                }
            }
        }

    if (parent) {
		parent->children.push_back(newNode);
	} else {
		nodes.push_back(newNode);
	}
}

vks::ModelDescriptor::ModelDescriptor(const boost::uuids::uuid& assetId, DescriptorManager* assetHandleManager,am::ModelData modelData,VulkanContext& vulkanContext) : IVulkanDescriptor(assetId, vulkanContext)
{
    loadNode(assetHandleManager,nullptr, modelData.rootNode, *this,vulkanContext);
}

vks::ModelDescriptor::~ModelDescriptor()
{

}
