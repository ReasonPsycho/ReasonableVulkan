//
// Created by redkc on 07/08/2025.
//

#ifndef MESHDATA_H
#define MESHDATA_H
#include "Vertex.hpp"


namespace am
{
    struct MeshData
    {
        std::vector<am::Vertex> vertices;
        std::vector<unsigned int> indices;
        std::shared_ptr<am::AssetInfo> material;
    };
}
#endif //MESHDATA_H
