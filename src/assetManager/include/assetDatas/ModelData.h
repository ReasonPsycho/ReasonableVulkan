//
// Created by redkc on 07/08/2025.
//

#ifndef MODELDATA_H
#define MODELDATA_H
#include "Node.h"


namespace am
{
    struct ModelData
    {
        am::Node rootNode; // Root node shouldn't actually hold any models since the blender will not have any prob should think about refactoring this
        glm::vec3 boundingBoxMin;
        glm::vec3 boundingBoxMax;
    };
}
#endif //MODELDATA_H
