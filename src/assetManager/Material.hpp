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
    static inline AssetFactoryRegistry::Registrar<Material> registrar{AssetType::Material};
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
        textureFactoryContext.assetType = AssetType::Texture;
        textureFactoryContext.assimpIndex = 0;
            
       aiString path;  // Create an actual aiString object, not a pointer
        material->GetTexture(aiTextureType_DIFFUSE, 0, &path, nullptr, nullptr, nullptr);
        textureFactoryContext.path = path.C_Str();
        auto diffuseResult = assetFactoryContext.assetManager.registerAsset(&textureFactoryContext);
        if (!diffuseResult) {
            spdlog::error("Failed to load texture: " + textureFactoryContext.path);
            throw std::runtime_error("Failed to load texture: " + textureFactoryContext.path);
        }
        diffuse = diffuseResult.value();
        
        material->GetTexture(aiTextureType_SHININESS, 0,  &path, nullptr, nullptr, nullptr);
        textureFactoryContext.path = path.C_Str();
        auto specularResult = assetFactoryContext.assetManager.registerAsset(&textureFactoryContext);
        if (!specularResult) {
            spdlog::error("Failed to load texture: " + textureFactoryContext.path);
            throw std::runtime_error("Failed to load texture: " + textureFactoryContext.path);
        }
        specular = specularResult.value();
    }

    size_t calculateContentHash() const override;

    [[nodiscard]] AssetType getType() const override;
    
    std::shared_ptr<AssetInfo> diffuse;  
    std::shared_ptr<AssetInfo> specular;  
};

}


#endif //MATERIAL_HPP
