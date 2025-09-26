//
// Created by redkc on 07/08/2025.
//

#ifndef MESHDATA_H
#define MESHDATA_H
#include "VertexAsset.hpp"


namespace am
{
    struct MeshData
    {
        std::vector<am::VertexAsset> vertices;
        std::vector<unsigned int> indices;
        std::shared_ptr<am::AssetInfo> material;
    };
}
#endif //MESHDATA_H
