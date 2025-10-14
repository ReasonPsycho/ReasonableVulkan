//
// Created by redkc on 09.05.2025.
//

#include "../include/Asset.hpp"
#include "AssetManager.hpp"

#include <spdlog/spdlog.h>

#include "assets/ModelAsset.h"
#include "assets/materialAsset/MaterialAsset.hpp"
#include "assets/shaderAsset/ShaderAsset.h"
#include "assets/textureAsset/TextureAsset.h"

namespace am {

    AssetManager::AssetManager()
    {
        // Register all known asset types
        registerFactory(AssetType::Material,
            [](am::AssetFactoryData &factoryData) {
                return std::unique_ptr<MaterialAsset>(new MaterialAsset(factoryData));
            }
        );

        registerFactory(AssetType::Texture,
          [](am::AssetFactoryData &factoryData) {
              return std::unique_ptr<TextureAsset>(new TextureAsset(factoryData));
          }
      );

        registerFactory(AssetType::Shader,
        [](am::AssetFactoryData &factoryData) {
            return std::unique_ptr<ShaderAsset>(new ShaderAsset(factoryData));
        }
    );

        registerFactory(AssetType::Model,
          [](am::AssetFactoryData &factoryData) {
              return std::unique_ptr<ModelAsset>(new ModelAsset(factoryData));
          }
      );
        registerFactory(AssetType::Mesh,
          [](am::AssetFactoryData &factoryData) {
              return std::unique_ptr<MeshAsset>(new MeshAsset(factoryData));
          }
      );

        loadMetadataFromFile("metadatas.json");
    }

    AssetManager::~AssetManager() {
        saveMetadataToFile("metadatas.json");
    }



bool AssetManager::saveMetadataToFile(const std::string& filename) const {
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
            info->SerializeToJson(assetInfoObj, allocator);
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

bool AssetManager::loadMetadataFromFile(const std::string& filename) {
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
            auto assetInfo = AssetInfo::DeserializeFromJson(assetInfoValue);
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
            auto factoryIt = factories.find(factoryContext->assetType);
            if (factoryIt == factories.end()) {
                spdlog::error("No factory registered for asset type");
                throw std::runtime_error("No factory registered for asset type");
            }

            std::unique_ptr<Asset> newAsset = factoryIt->second(*factoryContext);
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


    void AssetManager::registerFactory(AssetType type, AssetFactory factory) {
        factories[type] = std::move(factory);
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

        auto factory = getFactory(assetInfo->second.get()->type);
        if (factory) {
            auto loadResult = factory(assetInfo->second.get()->assetFactoryData);
            if (!loadResult) {
                spdlog::error("Failed to load asset: {}", assetInfo->second.get()->path);
                throw std::runtime_error("Failed to load asset: " +  assetInfo->second.get()->path);
            }
            assets[id] = std::move(loadResult);
            assetInfo->second.get()->loadedAsset = assets[id].get();
            assetInfo->second.get()->isLoaded = true;
        }
        importer.FreeScene();

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
        auto it = factories.find(type);
        if(it != factories.end())
        {
            return it->second;
        }
        spdlog::error("Failed to get asset factory!");
        throw std::runtime_error("Failed to get asset factory!");

    }
}
