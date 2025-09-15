#include "MaterialAsset.hpp"

am::MaterialAsset::MaterialAsset(AssetFactoryData& assetFactoryData) : Asset(assetFactoryData) {
    AssetManager &assetManager = AssetManager::getInstance();
    auto scene = assetManager.importer.GetScene();

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        scene = assetManager.importer.ReadFile(assetFactoryData.path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            spdlog::error("Assimp error: " + std::string(assetManager.importer.GetErrorString()));
            throw std::runtime_error("Assimp error: " + std::string(assetManager.importer.GetErrorString()));
        }
    }

    auto* aiMaterial = scene->mMaterials[assetFactoryData.assimpIndex];
    if (aiMaterial) {
        extractPBRData(aiMaterial,assetFactoryData);
    } else {
        spdlog::error("Material not found in scene.");
        throw std::runtime_error("Material not found in scene.");
    }
}

void am::MaterialAsset::extractPBRData(const aiMaterial* aiMaterial,AssetFactoryData& assetFactoryData) {
    AssetManager &assetManager = AssetManager::getInstance();

    AssetFactoryData textureFactoryContext = assetFactoryData;
    textureFactoryContext.assetType = AssetType::Texture;

    // Load base color texture
    aiString path;
    if (aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS) {
        textureFactoryContext.path = path.C_Str();
        auto result = assetManager.registerAsset(&textureFactoryContext);
        if (result) {
            data.baseColorTexture = result.value();
        }
    }

    // Load the legacy diffuse texture (can overlap with `baseColorTexture` for backward compatibility)
    if (aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS) {
        textureFactoryContext.path = path.C_Str();
        auto result = assetManager.registerAsset(&textureFactoryContext);
        if (result) {
            data.diffuseTexture = result.value();
        }
    }

    // Load metallic-roughness texture
    if (aiMaterial->GetTexture(aiTextureType_UNKNOWN, 0, &path) == AI_SUCCESS) { // Custom PBR data
        textureFactoryContext.path = path.C_Str();
        auto result = assetManager.registerAsset(&textureFactoryContext);
        if (result) {
            data.metallicRoughnessTexture = result.value();
        }
    }

    // Extract numeric PBR values
    float metallic = 1.0f, roughness = 1.0f;
    aiMaterial->Get(AI_MATKEY_METALLIC_FACTOR, metallic);
    aiMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
    data.metallicFactor = metallic;
    data.roughnessFactor = roughness;

    // Normal map
    if (aiMaterial->GetTexture(aiTextureType_NORMALS, 0, &path) == AI_SUCCESS) {
        textureFactoryContext.path = path.C_Str();
        auto result = assetManager.registerAsset(&textureFactoryContext);
        if (result) {
            data.normalTexture = result.value();
        }
    }

    // Occlusion map
    if (aiMaterial->GetTexture(aiTextureType_LIGHTMAP, 0, &path) == AI_SUCCESS) {
        textureFactoryContext.path = path.C_Str();
        auto result = assetManager.registerAsset(&textureFactoryContext);
        if (result) {
            data.occlusionTexture = result.value();
        }
    }

    // Emissive map
    if (aiMaterial->GetTexture(aiTextureType_EMISSIVE, 0, &path) == AI_SUCCESS) {
        textureFactoryContext.path = path.C_Str();
        auto result = assetManager.registerAsset(&textureFactoryContext);
        if (result) {
            data.emissiveTexture = result.value();
        }
    }

    // Alpha cutoff and transparency
    float opacity;
    if (aiMaterial->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS) {
        data.alphaCutoff = opacity;
        data.isOpaque = opacity >= 1.0f;
    }
}

size_t am::MaterialAsset::calculateContentHash() const {
    size_t hash = 0;

    if (data.baseColorTexture) {
        hash ^= data.baseColorTexture->contentHash + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }
    if (data.metallicRoughnessTexture) {
        hash ^= data.metallicRoughnessTexture->contentHash + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }
    return hash;
}

am::AssetType am::MaterialAsset::getType() const {
    return am::AssetType::Material;
}