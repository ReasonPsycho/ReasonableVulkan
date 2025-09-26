//
// Created by redkc on 10/08/2025.
//

#ifndef NODE_HANDLE_H
#define NODE_HANDLE_H
#include <cstdint>
#include <string>
#include <vector>
#include <glm/fwd.hpp>

#include "../DescriptorManager.h"
#include "descriptors/meshDescriptor/MeshDescriptor.h"
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
        std::vector<MeshDescriptor *> meshes;

        NodeHandle();
        ~NodeHandle();
    };
}


#endif //NODE_H
