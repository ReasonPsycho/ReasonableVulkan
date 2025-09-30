//
// Created by redkc on 10/08/2025.
//

#ifndef NODE_HANDLE_H
#define NODE_HANDLE_H
#include <string>
#include <vector>

#include "glm/gtc/quaternion.hpp"
namespace am
{
    class Mesh;
}

namespace vks
{
    class MeshDescriptor;

    struct NodeDescriptorStruct {
        std::string name;
        glm::mat4 matrix;
        NodeDescriptorStruct *parent;
        std::vector<NodeDescriptorStruct *> children;
        std::vector<MeshDescriptor *> meshes;

        NodeDescriptorStruct();
        ~NodeDescriptorStruct();
    };

    inline NodeDescriptorStruct::NodeDescriptorStruct():
    parent(nullptr),
    name(""),
    matrix(glm::mat4(1.0f))
    {
    }
}


#endif //NODE_H
