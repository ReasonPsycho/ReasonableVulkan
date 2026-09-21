#include "MaterialAsset.hpp"
#include "../../AssetManager.hpp"
#include "../../JsonHelpers.hpp"

am::MaterialAsset::MaterialAsset(const boost::uuids::uuid& id, std::string path) : Asset(id, path) {
}

am::MaterialAsset::MaterialAsset(const boost::uuids::uuid& id, ImportContext assetFactoryData) : Asset(id, assetFactoryData) {
    AssetManager &assetManager = AssetManager::getInstance();
    auto scene = assetManager.importer.GetScene();

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        scene = assetManager.importer.ReadFile(assetFactoryData.importPath, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
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

am::MaterialAsset::MaterialAsset(const std::string& path, AssetFormat format) : Asset(path, format) {
    if (format == AssetFormat::Json) {
        rapidjson::Document document;
        if (!loadJsonFromFile(path, document)) {
            spdlog::error("Failed to load MaterialAsset from JSON: {}", path);
            return;
        }

        if (document.HasMember("uuid") && document["uuid"].IsString()) {
            id = boost::uuids::string_generator()(document["uuid"].GetString());
        }

        DeserializeAssetData(data, document, &AssetManager::getInstance());
    }
}

void am::MaterialAsset::extractPBRData(const aiMaterial* aiMaterial,ImportContext& assetFactoryData) {
    AssetManager &assetManager = AssetManager::getInstance();

    ImportContext textureFactoryContext = assetFactoryData;
    textureFactoryContext.assetType = AssetType::Texture;

    // Load base color texture
    aiString path;
    if (aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS) {
        textureFactoryContext.importPath = path.C_Str();
        auto result = assetManager.registerAsset(textureFactoryContext);
        if (result) {
            data.baseColorTexture = assetManager.getAssetInfo(result.value()).value_or(nullptr);
        }
    }

    // Load the legacy diffuse texture (can overlap with `baseColorTexture` for backward compatibility)
    if (aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS) {
        textureFactoryContext.importPath = path.C_Str();
        auto result = assetManager.registerAsset(textureFactoryContext);
        if (result) {
            data.diffuseTexture = assetManager.getAssetInfo(result.value()).value_or(nullptr);
        }
    }

    // Load metallic-roughness texture
    if (aiMaterial->GetTexture(aiTextureType_UNKNOWN, 0, &path) == AI_SUCCESS) { // Custom PBR data
        textureFactoryContext.importPath = path.C_Str();
        auto result = assetManager.registerAsset(textureFactoryContext);
        if (result) {
            data.metallicRoughnessTexture = assetManager.getAssetInfo(result.value()).value_or(nullptr);
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
        textureFactoryContext.importPath = path.C_Str();
        auto result = assetManager.registerAsset(textureFactoryContext);
        if (result) {
            data.normalTexture = assetManager.getAssetInfo(result.value()).value_or(nullptr);
        }
    }

    // Occlusion map
    if (aiMaterial->GetTexture(aiTextureType_LIGHTMAP, 0, &path) == AI_SUCCESS) {
        textureFactoryContext.importPath = path.C_Str();
        auto result = assetManager.registerAsset(textureFactoryContext);
        if (result) {
            data.occlusionTexture = assetManager.getAssetInfo(result.value()).value_or(nullptr);
        }
    }

    // Emissive map
    if (aiMaterial->GetTexture(aiTextureType_EMISSIVE, 0, &path) == AI_SUCCESS) {
        textureFactoryContext.importPath = path.C_Str();
        auto result = assetManager.registerAsset(textureFactoryContext);
        if (result) {
            data.emissiveTexture = assetManager.getAssetInfo(result.value()).value_or(nullptr);
        }
    }

    // Alpha cutoff and transparency
    float opacity;
    if (aiMaterial->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS) {
        data.alphaCutoff = opacity;
        data.isOpaque = opacity >= 1.0f;
    }
}

void am::MaterialAsset::SaveAssetToJson(rapidjson::Document& document) {
    auto& allocator = document.GetAllocator();
    if (!document.IsObject()) {
        document.SetObject();
    }

    document.AddMember("uuid", rapidjson::Value(boost::uuids::to_string(id).c_str(), allocator), allocator);
    SerializeAssetData(data, document, allocator);
}

size_t am::MaterialAsset::calculateContentHash() const {
    return CalculateReflectedContentHash(data);
}

am::AssetType am::MaterialAsset::getType() const {
    return am::AssetType::Material;
}