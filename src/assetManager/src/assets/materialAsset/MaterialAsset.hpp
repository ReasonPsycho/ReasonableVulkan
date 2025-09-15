//
// Created by redkc on 20.05.2025.
//

#ifndef MATERIAL_HPP
#define MATERIAL_HPP
#include <assimp/Importer.hpp>

#include "../../../include/Asset.hpp"
#include <assimp/scene.h>
#include "../../AssetFactoryRegistry.hpp"
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

#include "../textureAsset/TextureAsset.h"
#include "assetDatas/MaterialData.h"

namespace am {

class MaterialAsset : public am::Asset {
    static inline AssetFactoryRegistry::Registrar<MaterialAsset> registrar{AssetType::Material};
public:
    explicit MaterialAsset(AssetFactoryData &assetFactoryData);


    size_t calculateContentHash() const override;
    [[nodiscard]] AssetType getType() const override;

    void* getAssetData() override { return &data; }
private:
    MaterialData data;
    void extractPBRData(const aiMaterial* aiMaterial,AssetFactoryData& assetFactoryData);
};

}


#endif //MATERIAL_HPP
