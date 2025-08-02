//
// Created by redkc on 02/08/2025.
//

#ifndef TRANSFORMNODE_H
#define TRANSFORMNODE_H

#endif //TRANSFORMNODE_H

namespace engine::ecs
{
    struct TransformNode {
        Entity parent = MAX_ENTITIES;
        std::vector<Entity> children;
    };
}