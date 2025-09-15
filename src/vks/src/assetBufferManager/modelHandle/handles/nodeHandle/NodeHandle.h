//
// Created by redkc on 10/08/2025.
//

#ifndef NODE_HANDLE_H
#define NODE_HANDLE_H
#include <cstdint>
#include <string>
#include <vector>
#include <glm/fwd.hpp>

#include "../../../AssetHandleManager.h"
#include "../meshHandle/MeshHandle.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
namespace am
{
    class Mesh;
}

namespace vks
{
    struct NodeHandle {
        std::string name;
        glm::mat4 matrix;
        NodeHandle *parent;
        std::vector<NodeHandle *> children;
        std::vector<MeshHandle *> meshes;

        NodeHandle(AssetHandleManager* assetHandleManager,am::Node node);
        ~NodeHandle();
    };

    inline NodeHandle::NodeHandle(AssetHandleManager* assetHandleManager,am::Node node)
    {
        // Handle all meshes in this node
        for (auto mesh : node.meshes)
        {
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

        // Recursively process all child nodes
        for (auto* child : node->children)
        {
            prepareNodeDescriptor(child, descriptorSetLayout);
        }

    }
}


#endif //NODE_H
