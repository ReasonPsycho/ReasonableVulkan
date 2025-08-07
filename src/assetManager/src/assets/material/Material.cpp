//
// Created by redkc on 20.05.2025.
//

#include "Material.hpp"

am::Material::Material(AssetFactoryData& assetFactoryData): Asset(assetFactoryData)
{
    AssetManager &assetManager = AssetManager::getInstance();
    auto scene = assetManager.importer.GetScene();

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        scene = assetManager.importer.ReadFile(assetFactoryData.path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            spdlog::error("Assimp error: " + std::string(assetManager.importer.GetErrorString()));
            throw std::runtime_error("Assimp error: " + std::string(assetManager.importer.GetErrorString()));
        }
    }

    auto* aiMaterial = scene->mMaterials[assetFactoryData.assimpIndex];

    AssetFactoryData textureFactoryContext{assetFactoryData};
    textureFactoryContext.assetType = AssetType::Texture;
    textureFactoryContext.assimpIndex = 0;

    aiString path;  // Create an actual aiString object, not a pointer
    aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &path, nullptr, nullptr, nullptr);
    textureFactoryContext.path = path.C_Str();
    auto diffuseResult = assetManager.registerAsset(&textureFactoryContext);
    if (!diffuseResult) {
        spdlog::error("Failed to load texture: " + textureFactoryContext.path);
        throw std::runtime_error("Failed to load texture: " + textureFactoryContext.path);
    }
    data.diffuse = diffuseResult.value();

    aiMaterial->GetTexture(aiTextureType_SHININESS, 0,  &path, nullptr, nullptr, nullptr);
    textureFactoryContext.path = path.C_Str();
    auto specularResult = assetManager.registerAsset(&textureFactoryContext);
    if (!specularResult) {
        spdlog::error("Failed to load texture: " + textureFactoryContext.path);
        throw std::runtime_error("Failed to load texture: " + textureFactoryContext.path);
    }
    data.specular = specularResult.value();
}

size_t am::Material::calculateContentHash() const {
    size_t hash = 0;

    hash ^= data.diffuse->contentHash + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= data.specular->contentHash + 0x9e3779b9 + (hash << 6) + (hash >> 2);

    return hash;
}

am::AssetType am::Material::getType() const {
    return am::AssetType::Material;
}
