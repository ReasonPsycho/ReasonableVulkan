//
// Created by redkc on 07/08/2025.
//

#ifndef MESHDATA_H
#define MESHDATA_H
#include "VertexHandle.hpp"


namespace am
{
    struct MeshData
    {
        std::vector<am::VertexHandle> vertices;
        std::vector<unsigned int> indices;
        std::shared_ptr<am::AssetInfo> material;
    };
}
#endif //MESHDATA_H
