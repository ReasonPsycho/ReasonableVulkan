//
// Created by redkc on 20.05.2025.
//

#ifndef MATERIAL_HPP
#define MATERIAL_HPP
#include <assimp/Importer.hpp>

#include "Asset.hpp"
#include <assimp/scene.h>
#include "AssetFactoryRegistry.hpp"
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

#include "Texture.h"

namespace am {
    
class Material : public am::Asset {
public:
    explicit Material(AssetFactoryData &assetFactoryData)
        : Asset(assetFactoryData) {
        if (!assetFactoryData.scene) {
            Assimp::Importer importer;
            const auto *scene = importer.ReadFile(assetFactoryData.path,
                                                  aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                                                  // aiProcess_FlipUVs | 
                                                  aiProcess_CalcTangentSpace);
            assetFactoryData.scene = std::make_optional(const_cast<aiScene *>(scene));
        }

        ExtractMateralData(assetFactoryData, assetFactoryData.scene.value());
    }

    void ExtractMateralData(AssetFactoryData assetFactoryContext, const aiScene *scene) {
        auto* material = scene->mMaterials[assetFactoryContext.assimpIndex];

        AssetFactoryData textureFactoryContext{assetFactoryContext};
        aiString *path = nullptr;

        material->GetTexture(aiTextureType_DIFFUSE, 0, path, nullptr, nullptr, nullptr);
        textureFactoryContext.path = path->C_Str();
        diffuse = assetFactoryContext.assetManager.registerAsset(&textureFactoryContext);

        material->GetTexture(aiTextureType_SHININESS, 0, path, nullptr, nullptr, nullptr);
        textureFactoryContext.path = path->C_Str();
        specular = assetFactoryContext.assetManager.registerAsset(&textureFactoryContext);
    }

    size_t calculateContentHash() const override;

    [[nodiscard]] AssetType getType() const override;
    
    std::shared_ptr<AssetInfo> diffuse;  
    std::shared_ptr<AssetInfo> specular;  
};

}


#endif //MATERIAL_HPP
