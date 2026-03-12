//
// Created by redkc on 09.05.2025.
//

#include "../include/Asset.hpp"
#include "AssetManager.hpp"

#include <spdlog/spdlog.h>

#include "assets/ModelAsset.h"
#include "assets/materialAsset/MaterialAsset.hpp"
#include "assets/shaderAsset/ShaderAsset.h"
#include "assets/shaderProgram/ShaderProgramAsset.h"
#include "assets/textureAsset/TextureAsset.h"

namespace am {

    AssetManager::AssetManager()
    {
        RegisterAssetType<MaterialAsset>();
        RegisterAssetType<TextureAsset>();
        RegisterAssetType<ShaderAsset>();
        RegisterAssetType<ShaderProgramAsset>();
        RegisterAssetType<ModelAsset>();
        RegisterAssetType<MeshAsset>();

        loadRegistryMetadataFromFile("metadatas.json");
    }

    AssetManager::~AssetManager() {
        saveRegistryMetadataToFile("metadatas.json");
    }



bool AssetManager::saveRegistryMetadataToFile(const std::string& filename) const {
    try {
        rapidjson::Document document;
        document.SetObject();
        auto& allocator = document.GetAllocator();

        // Add encoding information
        rapidjson::Value encodingInfo(rapidjson::kObjectType);
        encodingInfo.AddMember("encoding", "UTF-8", allocator);
        encodingInfo.AddMember("version", "1.0", allocator);
        document.AddMember("_meta", encodingInfo, allocator);

        // Create metadata array
        rapidjson::Value metadataArray(rapidjson::kArrayType);

        for (const auto& [uuid, info] : metadata) {
            rapidjson::Value assetInfoObj(rapidjson::kObjectType);
            info->SerializeAssetInfoToJson(assetInfoObj, allocator);
            metadataArray.PushBack(assetInfoObj, allocator);
        }

        document.AddMember("metadata", metadataArray, allocator);

        // Convert to string with pretty printing
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        writer.SetIndent(' ', 2);
        document.Accept(writer);

        // Save to file
        std::ofstream ofs(filename, std::ios::out | std::ios::binary);
        if (!ofs.is_open()) {
            spdlog::error("Failed to open file for writing: {}", filename);
            return false;
        }

        // Write UTF-8 BOM
        const char bom[3] = { static_cast<char>(0xEF), static_cast<char>(0xBB), static_cast<char>(0xBF) };
        ofs.write(bom, 3);

        // Write the JSON content
        ofs.write(buffer.GetString(), buffer.GetSize());
        ofs.close();

        return true;
    } catch (const std::exception& e) {
        spdlog::error("Error saving metadata to file: {}", e.what());
        return false;
    }
}

bool AssetManager::loadRegistryMetadataFromFile(const std::string& filename) {
    try {
        std::ifstream ifs(filename, std::ios::binary);
        if (!ifs.is_open()) {
            spdlog::error("Failed to open file for reading: {}", filename);
            return false;
        }

        // Skip UTF-8 BOM if present
        char bom[3];
        ifs.read(bom, 3);
        if (!(bom[0] == static_cast<char>(0xEF) &&
              bom[1] == static_cast<char>(0xBB) &&
              bom[2] == static_cast<char>(0xBF))) {
            ifs.seekg(0);
        }

        std::string json_content((std::istreambuf_iterator<char>(ifs)),
                               std::istreambuf_iterator<char>());

        rapidjson::Document document;
        document.Parse(json_content.c_str());

        if (!document.IsObject() || !document.HasMember("metadata") ||
            !document["metadata"].IsArray()) {
            spdlog::error("Invalid metadata file format");
            return false;
        }

        // Clear existing data
        metadata.clear();
        pathToUUIDs.clear();
        assets.clear();

        // Load metadata
        const auto& metadataArray = document["metadata"].GetArray();
        for (const auto& assetInfoValue : metadataArray) {
            auto assetInfo = AssetInfo::DeserializeAssetInfoFromJson(assetInfoValue);
            auto infoPtr = std::make_shared<AssetInfo>(std::move(assetInfo));
            metadata[infoPtr->id] = infoPtr;
            pathToUUIDs[infoPtr->path].push_back(infoPtr->id);
        }

        return true;
    } catch (const std::exception& e) {
        spdlog::error("Error loading metadata from file: {}", e.what());
        return false;
    }
}

    std::optional<std::shared_ptr<AssetInfo>> AssetManager::registerAsset(AssetFactoryData *factoryContext) {
        try
        {
            // First check if we already have this factoryContext
            auto uuids = getUUIDsByPath(factoryContext->path);
            for (auto uuid : uuids)
            {
                auto info = getAssetInfo(uuid);
                if (info.value()->assetFactoryData == *factoryContext ) {
                    return info.value();
                }
            }

            // Create and load the asset to calculate its hash
            auto factory = getFactory(factoryContext->assetType);
            if (!factory) {
                spdlog::error("No factory registered for asset type");
                throw std::runtime_error("No factory registered for asset type");
            }

            std::unique_ptr<Asset> newAsset = factory(*factoryContext);
            size_t contentHash = newAsset->calculateContentHash();

            // Check if we have an asset with the same content hash
            auto existingAsset = std::find_if(metadata.begin(), metadata.end(),
                                              [contentHash](const auto &pair) {
                                                  return pair.second->contentHash == contentHash;
                                              });

            if (existingAsset != metadata.end()) {
                // We found an asset with the same content
                return existingAsset->second;
            }

            auto id = boost::uuids::random_generator()();
            // If we get here, this is a new unique asset
            auto info = std::make_shared<AssetInfo>(id, factoryContext->path, factoryContext->assetType, contentHash,
                                                    *factoryContext);
            info->isLoaded = true;

            metadata.insert(std::make_pair(id, info));
            assets[id] = std::move(newAsset);
            info->loadedAsset = assets[id].get();
            pathToUUIDs[factoryContext->path].push_back(id);

            return info;
        }
            catch (const std::exception& e) {
                return std::nullopt;
            }
        }

    std::optional<std::shared_ptr<AssetInfo>> AssetManager::registerAsset(std::string path)
    {
        std::filesystem::path fsPath(path);
        std::string extension = fsPath.extension().string(); // Includes the dot, e.g. ".fbx"

        // Example usage
        am::AssetType type = am::GetAssetTypeFromExtension(extension);

        AssetFactoryData* assetFactoryData = new AssetFactoryData(path, type, 0);
        return registerAsset(assetFactoryData);
    }


    AssetManager &AssetManager::getInstance() {
        static AssetManager instance;
        return instance;
    }

    std::optional<std::shared_ptr<AssetInfo> > AssetManager::getAssetInfo(const boost::uuids::uuid &id) const {
        auto it = metadata.find(id);
        if (it != metadata.end()) return it->second;
        spdlog::error("No asset found!");
        return std::nullopt;
    }

    std::vector<boost::uuids::uuid> AssetManager::getUUIDsByPath(const std::string &path) const {
        auto it = pathToUUIDs.find(path);
        if (it != pathToUUIDs.end()) {
            return it->second;
        }
        return {};
    }

    std::optional<Asset *> AssetManager::getAsset(const boost::uuids::uuid &id) {
        auto asset = assets.find(id);
        if (asset != assets.end()) return asset->second.get();

        auto assetInfo = metadata.find(id);
        if (assetInfo == metadata.end()) return std::nullopt;

        auto factory = getFactory(assetInfo->second->type);
        if (factory) {
            auto assetPtr = factory(assetInfo->second->assetFactoryData);
            if (!assetPtr)
            {
                spdlog::error("Failed to load asset: {}", assetInfo->second->path);
                throw std::runtime_error("Failed to load asset: " +  assetInfo->second->path);
            }

            std::string filename = assetInfo->second->path;

            size_t lastDot = filename.find_last_of('.');
            if (lastDot != std::string::npos)
                filename = filename.substr(0, lastDot) + ".meta";
            else
                filename += ".meta"; // fallback if no extension

            std::ifstream ifs(filename, std::ios::binary);
            if (!ifs.is_open()) {
                spdlog::info("Unable to read metadata for file: {}, creating new.", filename);

                rapidjson::Document document;
                document.SetObject();
                auto& allocator = document.GetAllocator();

                // Add encoding information
                rapidjson::Value encodingInfo(rapidjson::kObjectType);
                encodingInfo.AddMember("encoding", "UTF-8", allocator);
                encodingInfo.AddMember("version", "1.0", allocator);
                document.AddMember("_meta", encodingInfo, allocator);

                assetPtr->SaveAssetMetadata(document);

                // Convert to string with pretty printing
                rapidjson::StringBuffer buffer;
                rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
                writer.SetIndent(' ', 2);
                document.Accept(writer);

                // Save to file
                std::ofstream ofs(filename, std::ios::out | std::ios::binary);
                if (!ofs.is_open()) {
                    spdlog::error("Failed to open file for writing: {}", filename);
                    return std::nullopt;
                }

                // Write UTF-8 BOM
                const char bom[3] = { static_cast<char>(0xEF), static_cast<char>(0xBB), static_cast<char>(0xBF) };
                ofs.write(bom, 3);

                // Write the JSON content
                ofs.write(buffer.GetString(), buffer.GetSize());
                ofs.close();
            } else {
                // Skip UTF-8 BOM if present
                char bom[3];
                ifs.read(bom, 3);
                if (!(bom[0] == static_cast<char>(0xEF) &&
                      bom[1] == static_cast<char>(0xBB) &&
                      bom[2] == static_cast<char>(0xBF))) {
                    ifs.seekg(0);
                }

                std::string json_content((std::istreambuf_iterator<char>(ifs)),
                                       std::istreambuf_iterator<char>());

                rapidjson::Document document;
                document.Parse(json_content.c_str());

                assetPtr->LoadAssetMetadata(document);
            }
            
            assets[id] = std::move(assetPtr);

            assetInfo->second->loadedAsset = assets[id].get();
            assetInfo->second->isLoaded = true;
        }

        return assets[id].get();
    }

    std::vector<std::shared_ptr<am::AssetInfo>> AssetManager::getRegisteredAssets() const
    {
        std::vector<std::shared_ptr<am::AssetInfo>> result;
        result.reserve(metadata.size());
        for (const auto& [_, info] : metadata) {
            result.push_back(info);
        }
        return result;
    }

    std::vector<std::shared_ptr<am::AssetInfo>> AssetManager::getRegisteredAssets(AssetType type) const
    {
        std::vector<std::shared_ptr<am::AssetInfo>> result;
        for (const auto& [_, info] : metadata) {
            if (info->type == type)
            result.push_back(info);
        }
        return result;
    }

    AssetManager::AssetFactory AssetManager::getFactory(AssetType type) const
    {
        switch (type) {
            case AssetType::Mesh:
                return getFactory(std::type_index(typeid(MeshAsset)));
            case AssetType::Model:
                return getFactory(std::type_index(typeid(ModelAsset)));
            case AssetType::Texture:
                return getFactory(std::type_index(typeid(TextureAsset)));
            case AssetType::Shader:
                return getFactory(std::type_index(typeid(ShaderAsset)));
            case AssetType::ShaderProgram:
                return getFactory(std::type_index(typeid(ShaderProgramAsset)));
            case AssetType::Material:
                return getFactory(std::type_index(typeid(MaterialAsset)));
            default:
                spdlog::error("Failed to get asset factory for AssetType: {}", (int)type);
                throw std::runtime_error("Failed to get asset factory for AssetType!");
        }
    }

    AssetManager::AssetFactory AssetManager::getFactory(std::type_index type) const
    {
        auto it = factories.find(type);
        if(it != factories.end())
        {
            return it->second;
        }
        spdlog::error("Failed to get asset factory!");
        throw std::runtime_error("Failed to get asset factory!");
    }
}
