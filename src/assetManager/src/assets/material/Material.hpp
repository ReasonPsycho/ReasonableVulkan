//
// Created by redkc on 20.05.2025.
//

#ifndef MATERIAL_HPP
#define MATERIAL_HPP
#include <assimp/Importer.hpp>

#include "../../Asset.hpp"
#include <assimp/scene.h>
#include "../../AssetFactoryRegistry.hpp"
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

#include "../texture/Texture.h"

namespace am {
    
class Material : public am::Asset {
    static inline AssetFactoryRegistry::Registrar<Material> registrar{AssetType::Material};
public:
    explicit Material(AssetFactoryData &assetFactoryData);

    size_t calculateContentHash() const override;

    [[nodiscard]] AssetType getType() const override;
    
    std::shared_ptr<AssetInfo> diffuse;  
    std::shared_ptr<AssetInfo> specular;  
};

}


#endif //MATERIAL_HPP
