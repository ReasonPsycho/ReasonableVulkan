#include "MaterialAsset.hpp"
#include "../../AssetManager.hpp"
#include "../../JsonHelpers.hpp"

am::MaterialAsset::MaterialAsset(const boost::uuids::uuid& id) : Asset(id) {
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

am::MaterialAsset::MaterialAsset(const boost::uuids::uuid& id, const std::string& path, AssetFormat format) : Asset(id, path, format) {
    if (format == AssetFormat::Json) {
        rapidjson::Document document;
        if (!loadJsonFromFile(path, document)) {
            spdlog::error("Failed to load MaterialAsset from JSON: {}", path);
            return;
        }

        if (document.HasMember("uuid") && document["uuid"].IsString()) {
            std::string savedUuidStr = document["uuid"].GetString();
            boost::uuids::uuid savedUuid = boost::uuids::string_generator()(savedUuidStr);
                if (savedUuid != id) {
                    spdlog::warn("Material asset UUID mismatch in {}: expected {}, got {}", path.c_str(), boost::uuids::to_string(id).c_str(), savedUuidStr.c_str());
                }
        }

        AssetManager &assetManager = AssetManager::getInstance();

        auto loadTexture = [&](const char* key, std::shared_ptr<AssetInfo>& target) {
            if (document.HasMember(key) && document[key].IsString()) {
                std::string textureUuidStr = document[key].GetString();
                try {
                    boost::uuids::uuid textureId = boost::uuids::string_generator()(textureUuidStr);
                    target = assetManager.getAssetInfo(textureId).value_or(nullptr);
                    if (!target) {
                        spdlog::warn("Texture asset with UUID {} not found for material {}", textureUuidStr.c_str(), path.c_str());
                    }
                } catch (const std::exception& e) {
                    spdlog::error("Failed to parse texture UUID {} for material {}: {}", textureUuidStr.c_str(), path.c_str(), e.what());
                }
            }
        };

        loadTexture("baseColorTexture", data.baseColorTexture);
        loadTexture("diffuseTexture", data.diffuseTexture);
        loadTexture("metallicRoughnessTexture", data.metallicRoughnessTexture);
        loadTexture("specularGlossinessTexture", data.specularGlossinessTexture);
        loadTexture("normalTexture", data.normalTexture);
        loadTexture("occlusionTexture", data.occlusionTexture);
        loadTexture("emissiveTexture", data.emissiveTexture);

        auto loadVec4 = [&](const char* key, glm::vec4& vec) {
            if (document.HasMember(key) && document[key].IsArray() && document[key].Size() == 4) {
                vec.x = document[key][0].GetFloat();
                vec.y = document[key][1].GetFloat();
                vec.z = document[key][2].GetFloat();
                vec.w = document[key][3].GetFloat();
            }
        };

        auto loadVec3 = [&](const char* key, glm::vec3& vec) {
            if (document.HasMember(key) && document[key].IsArray() && document[key].Size() == 3) {
                vec.x = document[key][0].GetFloat();
                vec.y = document[key][1].GetFloat();
                vec.z = document[key][2].GetFloat();
            }
        };

        loadVec4("baseColorFactor", data.baseColorFactor);
        if (document.HasMember("metallicFactor") && document["metallicFactor"].IsNumber()) data.metallicFactor = document["metallicFactor"].GetFloat();
        if (document.HasMember("roughnessFactor") && document["roughnessFactor"].IsNumber()) data.roughnessFactor = document["roughnessFactor"].GetFloat();
        loadVec3("specularFactor", data.specularFactor);
        loadVec3("diffuseFactor", data.diffuseFactor);
        if (document.HasMember("glossinessFactor") && document["glossinessFactor"].IsNumber()) data.glossinessFactor = document["glossinessFactor"].GetFloat();
        if (document.HasMember("occlusionStrength") && document["occlusionStrength"].IsNumber()) data.occlusionStrength = document["occlusionStrength"].GetFloat();
        loadVec3("emissiveFactor", data.emissiveFactor);
        if (document.HasMember("alphaCutoff") && document["alphaCutoff"].IsNumber()) data.alphaCutoff = document["alphaCutoff"].GetFloat();
        if (document.HasMember("isOpaque") && document["isOpaque"].IsBool()) data.isOpaque = document["isOpaque"].GetBool();
        if (document.HasMember("useSpecularGlossiness") && document["useSpecularGlossiness"].IsBool()) data.useSpecularGlossiness = document["useSpecularGlossiness"].GetBool();
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

    auto addTexture = [&](const char* key, std::shared_ptr<AssetInfo>& texture) {
        if (texture) {
            std::string uuidStr = boost::uuids::to_string(texture->id);
            document.AddMember(rapidjson::StringRef(key), rapidjson::Value(uuidStr.c_str(), allocator), allocator);
        }
    };

    addTexture("baseColorTexture", data.baseColorTexture);
    addTexture("diffuseTexture", data.diffuseTexture);
    addTexture("metallicRoughnessTexture", data.metallicRoughnessTexture);
    addTexture("specularGlossinessTexture", data.specularGlossinessTexture);
    addTexture("normalTexture", data.normalTexture);
    addTexture("occlusionTexture", data.occlusionTexture);
    addTexture("emissiveTexture", data.emissiveTexture);

    auto addVec4 = [&](const char* key, const glm::vec4& vec) {
        rapidjson::Value array(rapidjson::kArrayType);
        array.PushBack(vec.x, allocator);
        array.PushBack(vec.y, allocator);
        array.PushBack(vec.z, allocator);
        array.PushBack(vec.w, allocator);
        document.AddMember(rapidjson::StringRef(key), array, allocator);
    };

    auto addVec3 = [&](const char* key, const glm::vec3& vec) {
        rapidjson::Value array(rapidjson::kArrayType);
        array.PushBack(vec.x, allocator);
        array.PushBack(vec.y, allocator);
        array.PushBack(vec.z, allocator);
        document.AddMember(rapidjson::StringRef(key), array, allocator);
    };

    addVec4("baseColorFactor", data.baseColorFactor);
    document.AddMember("metallicFactor", data.metallicFactor, allocator);
    document.AddMember("roughnessFactor", data.roughnessFactor, allocator);
    addVec3("specularFactor", data.specularFactor);
    addVec3("diffuseFactor", data.diffuseFactor);
    document.AddMember("glossinessFactor", data.glossinessFactor, allocator);
    document.AddMember("occlusionStrength", data.occlusionStrength, allocator);
    addVec3("emissiveFactor", data.emissiveFactor);
    document.AddMember("alphaCutoff", data.alphaCutoff, allocator);
    document.AddMember("isOpaque", data.isOpaque, allocator);
    document.AddMember("useSpecularGlossiness", data.useSpecularGlossiness, allocator);
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