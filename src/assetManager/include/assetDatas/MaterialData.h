//
// Created by redkc on 07/08/2025.
//

#ifndef MATERIALDATA_H
#define MATERIALDATA_H
#include <memory>
#include "AssetInfo.hpp"

namespace am
{


struct MaterialData
{
    std::shared_ptr<am::AssetInfo> diffuse;
    std::shared_ptr<am::AssetInfo> specular;
};


}
#endif //MATERIALDATA_H
