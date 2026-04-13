//
// Created by redkc on 20.05.2025.
//

#ifndef MATERIAL_HPP
#define MATERIAL_HPP
#include <assimp/Importer.hpp>

#include "../../../include/Asset.hpp"
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

#include "../textureAsset/TextureAsset.h"
#include "assetDatas/MaterialData.h"

namespace am {

class MaterialAsset : public am::Asset {
public:
    explicit MaterialAsset(AssetFactoryData &assetFactoryData);

    void LoadAssetFromImport(AssetFactoryData assetFactoryData) override;
    void saveAssetToJson(std::string& json) override {}
    void LoadAssetFromJson(std::string& json) override {}


    size_t calculateContentHash() const override;
    [[nodiscard]] AssetType getType() const override;

    void SaveAssetMetadata(rapidjson::Document& document) override {}
    void LoadAssetMetadata(rapidjson::Document& document) override {}

    void* getAssetData() override { return &data; }
private:
    MaterialData data;
    void extractPBRData(const aiMaterial* aiMaterial,AssetFactoryData& assetFactoryData);
};

}


#endif //MATERIAL_HPP
