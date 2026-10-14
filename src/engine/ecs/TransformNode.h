//
// Created by redkc on 02/08/2025.
//

#ifndef TRANSFORMNODE_H
#define TRANSFORMNODE_H

#include <vector>
#include <cstdint>
#include "Types.h"

namespace engine::ecs
{
    struct TransformNode {
        Entity parent = MAX_ENTITIES;
        std::vector<Entity> children;
    };

    struct FlatTransformEntry
    {
        Entity entity;
        Entity parent;       // MAX_ENTITIES for root
        int32_t parentIndex; // -1 for root
        bool isDirty;
    };
}

#endif //TRANSFORMNODE_H