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
#include "assets/engineAssets/SceneAsset.h"
#include "assets/engineAssets/PrefabAsset.h"
#include "JsonHelpers.hpp"

namespace am {

    AssetManager::AssetManager() : AssetManagerInterface()
    {
        RegisterAssetType<MaterialAsset>();
        RegisterAssetType<TextureAsset>();
        RegisterAssetType<ShaderAsset>();
        RegisterAssetType<ShaderProgramAsset>();
        RegisterAssetType<ModelAsset>();
        RegisterAssetType<MeshAsset>();
        RegisterAssetType<SceneAsset>();
        RegisterAssetType<PrefabAsset>();

        loadRegistryMetadataFromFile("C:\\Users\\redkc\\CLionProjects\\ReasonableVulkan\\res\\metadatas.json");
    }

    AssetManager::~AssetManager() {
        saveRegistryMetadataToFile("C:\\Users\\redkc\\CLionProjects\\ReasonableVulkan\\res\\metadatas.json");
    }

    std::optional<boost::uuids::uuid> AssetManager::createAsset(AssetType assetType, std::string path) {
        std::filesystem::path p = std::filesystem::path(path).lexically_normal();

        std::string baseName = p.stem().string();
        std::string extension = p.extension().string();

        std::string lookUpName = baseName + GetExtensionFromAssetType(assetType);

        int counter = 1;

        while (lookupNamesToUUIDs.find(lookUpName) != lookupNamesToUUIDs.end())
        {
            lookUpName = baseName + "_" + std::to_string(counter) +  GetExtensionFromAssetType(assetType);
            counter++;
        }

        std::string normalizedPath = p.parent_path().string() + "/" + lookUpName;

        return initializeAsset(assetType, normalizedPath, lookUpName);
    }

    std::optional<boost::uuids::uuid> AssetManager::createAsset(AssetType assetType, string path, std::string lookupName) {
        std::filesystem::path p = std::filesystem::path(path).lexically_normal();

        if (lookupNamesToUUIDs.find(lookupName) != lookupNamesToUUIDs.end())
        {
            spdlog::error("Lookup name already exists");
            throw std::runtime_error("Lookup name already exists");
        }

        std::string normalizedPath = p.parent_path().string() + "/" + lookupName + GetExtensionFromAssetType(assetType);

        return initializeAsset(assetType, normalizedPath, lookupName);
    }

    std::optional<boost::uuids::uuid> AssetManager::initializeAsset(AssetType assetType, std::string path,
        std::string lookupName)
    {
        try
        {
            auto creator = getCreator(getTypeIndex(assetType));
            if (!creator) {
                spdlog::error("No creator registered for asset type");
                throw std::runtime_error("No creator registered for asset type");
            }
            auto id = boost::uuids::random_generator()();
            auto newAsset = creator(id);
            auto info = std::make_shared<AssetInfo>(id, path, assetType, 0,ImportContext("", assetType, 0), lookupName);
            if (GetEditorSavesToBin(assetType))
            {
                std::string additionalSufix = "";
                if (info.get()->type == AssetType::Shader)
                {
                    additionalSufix = GetShaderSufix(newAsset.get()->getAssetDataAs<ShaderData>()->stage);
                }
                std::string filename = GetBinPath(path, additionalSufix);
                info->path = filename;
                newAsset->SaveAssetToBin(filename);
                metadata.insert(std::make_pair(id, info));
            }else
            {
                rapidjson::Document doc;
                doc.SetObject();
                newAsset->SaveAssetToJson(doc);
                saveJsonToFile(path, doc);
                info->path = path;
                metadata.insert(std::make_pair(id, info));
            }
            lookupNamesToUUIDs.insert(std::make_pair(lookupName, id));
            assets[id] = std::move(newAsset);
            metadata[id]->loadedAsset = assets[id].get();
            return id;
        }
        catch (std::exception& e)
        {
            spdlog::error("Failed to create asset");
        }
        return boost::uuids::nil_uuid();
    }


    bool AssetManager::saveRegistryMetadataToFile(const std::string& filename) const {
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

    return saveJsonToFile(filename, document);
}

bool AssetManager::loadRegistryMetadataFromFile(const std::string& filename) {
    rapidjson::Document document;
    if (!loadJsonFromFile(filename, document)) {
        return false;
    }

    if (!document.IsObject() || !document.HasMember("metadata") ||
        !document["metadata"].IsArray()) {
        spdlog::error("Invalid metadata file format");
        return false;
    }

    // Clear existing data
    metadata.clear();
    lookupNamesToUUIDs.clear();
    assets.clear();

    // Load metadata
    const auto& metadataArray = document["metadata"].GetArray();
    for (const auto& assetInfoValue : metadataArray) {
        auto assetInfo = AssetInfo::DeserializeAssetInfoFromJson(assetInfoValue);
        auto infoPtr = std::make_shared<AssetInfo>(std::move(assetInfo));
        metadata[infoPtr->id] = infoPtr;
        lookupNamesToUUIDs[infoPtr->lookUpName] = infoPtr->id;
    }

    return true;
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

    std::optional<Asset*> AssetManager::getAsset(const boost::uuids::uuid& id)
    {
        auto it = assets.find(id);
        if (it != assets.end()) return it->second.get();

        auto assetInfo = metadata.find(id);
        if (assetInfo == metadata.end()) return std::nullopt;
        auto decodedAssetInfo = assetInfo->second;
        unique_ptr<Asset> assetNew;
        try
        {
            if (GetEditorSavesToBin(assetInfo->second->type))
            {
                auto binLoader = getLoader(getTypeIndex(assetInfo->second->type));
                if (!binLoader) {
                    spdlog::error("No factory registered for asset type");
                    throw std::runtime_error("No factory registered for asset type");
                }
                assetNew = binLoader(id, assetInfo->second->path, AssetFormat::Binary);
            }else
            {
                auto jsonLoader = getLoader(getTypeIndex(assetInfo->second->type));
                if (!jsonLoader) {
                    spdlog::error("No factory registered for asset type");
                    throw std::runtime_error("No factory registered for asset type");
                }
                assetNew = jsonLoader(id, assetInfo->second->path, AssetFormat::Json);
            }
            assetInfo->second->loadedAsset = assetNew.get();
            assetInfo->second->isLoaded = true;
            assets[id] = std::move(assetNew);
            return assets[id].get();
        }catch (const std::exception& e)
        {
            spdlog::error("Failed to load asset");
            return std::nullopt;
        }
        return std::nullopt;
    }

    void AssetManager::saveAsset(const boost::uuids::uuid id)
    {
        auto info = getAssetInfo(id);
        if (!info)
        {
            spdlog::error("No asset info found with id: {}", boost::uuids::to_string(id));
        }
        auto asset = getAsset(id);
        if (!asset)
        {
            spdlog::error("No asset found with id: {}", boost::uuids::to_string(id));
        }
        
        if (GetEditorSavesToBin(info.value()->type))
        {
            asset.value()->SaveAssetToBin(info.value()->path);
        } else {
            rapidjson::Document document;
            document.SetObject();

            asset.value()->SaveAssetToJson(document);

            auto& allocator = document.GetAllocator();

            // Add encoding information
            rapidjson::Value encodingInfo(rapidjson::kObjectType);
            encodingInfo.AddMember("encoding", "UTF-8", allocator);
            encodingInfo.AddMember("version", "1.0", allocator);
            document.AddMember("_meta", encodingInfo, allocator);

            saveJsonToFile(info.value()->path, document);
        }
    }

    void AssetManager::saveAsset(std::string lookupName)
    {
        auto uuid = getAssetUuid(lookupName);
        if (uuid) {
            saveAsset(uuid.value());
        } else {
            spdlog::error("No asset found with lookup name: {}", lookupName);
            return;
        }
    }


    std::optional<boost::uuids::uuid> AssetManager::registerAsset(std::string path, std::string lookUpName)
    {
        std::filesystem::path p = std::filesystem::path(path).lexically_normal();
        std::string normalizedPath = p.string();

        std::string extension = p.extension().string();

        if (lookupNamesToUUIDs.find(lookUpName) != lookupNamesToUUIDs.end())
        {
            spdlog::error("Lookup name already exists");
            throw std::runtime_error("Lookup name already exists");
        }

        ImportContext assetFactoryData(normalizedPath, GetAssetTypeFromExtension(extension), 0);
        return importAsset(assetFactoryData, lookUpName);
    }

    std::optional<boost::uuids::uuid> AssetManager::registerAsset(std::string path)
    {
        std::filesystem::path p = std::filesystem::path(path).lexically_normal();
        std::string normalizedPath = p.string();

        std::string baseName = p.stem().string();
        std::string extension = p.extension().string();

        std::string lookUpName = baseName + GetExtensionFromAssetType(GetAssetTypeFromExtension(extension));

        int counter = 1;

        while (lookupNamesToUUIDs.find(lookUpName) != lookupNamesToUUIDs.end())
        {
            lookUpName = baseName + "_" + std::to_string(counter) +  GetExtensionFromAssetType(GetAssetTypeFromExtension(extension));
            counter++;
        }

        ImportContext assetFactoryData(normalizedPath, GetAssetTypeFromExtension(extension), 0);
        return importAsset(assetFactoryData, lookUpName);
    }

    std::string incrementSuffix(const std::string& suffix)
    {
        if (suffix.empty())
            return "a";

        std::string result = suffix;
        int i = result.size() - 1;

        while (i >= 0)
        {
            if (result[i] < 'z')
            {
                result[i]++;
                return result;
            }
            result[i] = 'a';
            --i;
        }

        // overflow (e.g. "z" -> "aa", "zz" -> "aaa")
        return "a" + result;
    }

    std::optional<boost::uuids::uuid> AssetManager::registerAsset(ImportContext importContext)
    {
        importContext.importPath = std::filesystem::path(importContext.importPath).lexically_normal().string();
        std::filesystem::path p(importContext.importPath);

        std::string baseName = p.stem().string();
        std::string extension = p.extension().string();

        std::string baseLookupName = baseName + "_" + std::to_string(importContext.assimpIndex);
        std::string suffix = "";
        std::string lookUpName;

        while (true)
        {
            lookUpName = baseLookupName + suffix + GetExtensionFromAssetType(importContext.assetType);

            if (lookupNamesToUUIDs.find(lookUpName) == lookupNamesToUUIDs.end())
                break;

            suffix = incrementSuffix(suffix);
        }

        return importAsset(importContext, lookUpName);
    }



    std::optional<boost::uuids::uuid> AssetManager::getAssetUuid(std::string lookupName)
    {
        auto uuid = lookupNamesToUUIDs.find(lookupName);
        if (uuid == lookupNamesToUUIDs.end()) return std::nullopt;
        return uuid->second;
    }


    any AssetManager::getAssetData(const boost::uuids::uuid& id) {
        auto asset = getAsset(id);

        if (asset.has_value())
        {
            return asset.value()->getAssetData();
        }
        return std::nullopt;
    }

    any AssetManager::getAssetData(std::string lookupName)
    {
        auto it = lookupNamesToUUIDs.find(lookupName);
        if (it != lookupNamesToUUIDs.end() ) {
            return getAssetData(it->second);
        }
        return nullptr;
    }

    std::vector<std::string> AssetManager::getRegisteredAssetsNames() const
    {
        std::vector<std::string> result;
        result.reserve(lookupNamesToUUIDs.size());
        for (const auto& [name, uuids] : lookupNamesToUUIDs) {
            result.push_back(name);
        }
        return result;
    }

    std::vector<std::string> AssetManager::getRegisteredAssetsNames(AssetType type) const
    {
        std::vector<std::string> result;
        for (const auto& [_, info] : metadata) {
            if (info->type == type)
                result.push_back(info->lookUpName);
        }
        return result;
    }

    std::vector<boost::uuids::uuid> AssetManager::getRegisteredAssetsUuids() const
    {
        std::vector<boost::uuids::uuid> result;
        result.reserve(metadata.size());
        for (const auto& [_, info] : metadata) {
            result.push_back(info.get()->id);
        }
        return result;
    }

    std::vector<boost::uuids::uuid> AssetManager::getRegisteredAssetsUuids(AssetType type) const
    {
        std::vector<boost::uuids::uuid> result;
        for (const auto& [_, info] : metadata) {
            if (info->type == type)
                result.push_back(info.get()->id);
        }
        return result;
    }

    std::optional<boost::uuids::uuid> AssetManager::importAsset(ImportContext importContext, string lookUpName)
    {
        importContext.importPath = std::filesystem::path(importContext.importPath).lexically_normal().string();
        try
        {
            // Create and load the asset to calculate its hash
            auto factory = getImporter(getTypeIndex(importContext.assetType));
            if (!factory) {
                spdlog::error("No factory registered for asset type");
                throw std::runtime_error("No factory registered for asset type");
            }

            auto id = boost::uuids::random_generator()();
            std::unique_ptr<Asset> newAsset = factory(id, importContext);
            size_t contentHash = newAsset->calculateContentHash();

            // Check if we have an asset with the same content hash
            auto existingAsset = std::find_if(metadata.begin(), metadata.end(),
                                              [contentHash](const auto &pair) {
                                                  return pair.second->contentHash == contentHash;
                                              });

            if (existingAsset != metadata.end()) {
                // We found an asset with the same content
                return existingAsset->second.get()->id;
            }

            // If we get here, this is a new unique asset
            auto info = std::make_shared<AssetInfo>(id, importContext.importPath, importContext.assetType, contentHash,importContext, lookUpName);
            info->isLoaded = true;

            std::filesystem::path p = std::filesystem::path(importContext.importPath).lexically_normal();
            std::string baseName = p.stem().string();

            if (GetEditorSavesToBin(importContext.assetType))
            {
                std::string additionalSufix = "";
                if (info.get()->type == AssetType::Shader)
                {
                    additionalSufix = GetShaderSufix(newAsset.get()->getAssetDataAs<ShaderData>()->stage);
                }
                std::string filename = GetBinPath((p.parent_path() / (baseName + GetExtensionFromAssetType(importContext.assetType))).string(), additionalSufix);
                info->path = filename;
                newAsset->SaveAssetToBin(filename);
            } else {
                std::string filename = (p.parent_path() / (baseName + GetExtensionFromAssetType(importContext.assetType))).string();
                info->path = filename;
                auto jsonSaver = getJsonSaver(getTypeIndex(importContext.assetType));

                rapidjson::Document document;
                document.SetObject();
                auto& allocator = document.GetAllocator();

                // Add encoding information
                rapidjson::Value encodingInfo(rapidjson::kObjectType);
                encodingInfo.AddMember("encoding", "UTF-8", allocator);
                encodingInfo.AddMember("version", "1.0", allocator);
                document.AddMember("_meta", encodingInfo, allocator);

                jsonSaver(*newAsset, document);

                saveJsonToFile(filename, document);
            }

            //We are here so we can save everything

            metadata.insert(std::make_pair(id, info));
            assets[id] = std::move(newAsset);
            info->loadedAsset = assets[id].get();
            lookupNamesToUUIDs[lookUpName] = id;
            return id;
        }
        catch (std::exception& e)
        {
            spdlog::error("Failed to import asset");
        }
        return std::nullopt;
    }
    
    std::type_index AssetManager::getTypeIndex(AssetType type) const
    {
        switch (type) {
        case AssetType::Mesh:
            return std::type_index(typeid(MeshAsset));
        case AssetType::Model:
            return std::type_index(typeid(ModelAsset));
        case AssetType::Texture:
            return std::type_index(typeid(TextureAsset));
        case AssetType::Shader:
            return std::type_index(typeid(ShaderAsset));
        case AssetType::ShaderProgram:
            return std::type_index(typeid(ShaderProgramAsset));
        case AssetType::Material:
            return std::type_index(typeid(MaterialAsset));
        case AssetType::Scene:
            return std::type_index(typeid(SceneAsset));
        case AssetType::Prefab:
            return std::type_index(typeid(PrefabAsset));
        default:
            spdlog::error("Failed to get type_index for AssetType");
            throw std::runtime_error("Failed to get type_index for AssetType!");
        }
    }


    AssetManager::AssetImporter AssetManager::getImporter(std::type_index type) const
    {
        auto it = importers.find(type);
        if(it != importers.end())
        {
            return it->second;
        }
        spdlog::error("Failed to get asset factory!");
        throw std::runtime_error("Failed to get asset factory!");
    }

    AssetManager::AssetCreator AssetManager::getCreator(std::type_index type) const
    {
        auto it = creators.find(type);
        if(it != creators.end())
        {
            return it->second;
        }
        spdlog::error("Failed to get creator!");
        throw std::runtime_error("Failed to get creator!");
    }

    AssetManager::AssetJsonSaver AssetManager::getJsonSaver(std::type_index type) const
    {
        auto it = jsonSavers.find(type);
        if(it != jsonSavers.end())
        {
            return it->second;
        }
        spdlog::error("Failed to get json saver!");
        throw std::runtime_error("Failed to get json saver!");
    }

    AssetManager::AssetLoader AssetManager::getLoader(std::type_index type) const
    {
        auto it = loaders.find(type);
        if(it != loaders.end())
        {
            return it->second;
        }
        spdlog::error("Failed to get loader!");
        throw std::runtime_error("Failed to get loader!");
    }



    AssetManager::MetadataLoader AssetManager::getMetadataLoader(std::type_index type) const
    {
        auto it = metadataLoaders.find(type);
        if(it != metadataLoaders.end())
        {
            return it->second;
        }
        spdlog::error("Failed to get metadata loader!");
        throw std::runtime_error("Failed to get metadata loader!");
    }

    AssetManager::MetadataSaver AssetManager::getMetadataSaver(std::type_index type) const
    {
        auto it = metadataSavers.find(type);
        if(it != metadataSavers.end())
        {
            return it->second;
        }
        spdlog::error("Failed to get metadata saver!");
        throw std::runtime_error("Failed to get metadata saver!");
    }


}
