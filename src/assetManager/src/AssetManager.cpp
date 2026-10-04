//
// Created by redkc on 09.05.2025.
//

#include "../include/Asset.hpp"
#include "AssetManager.hpp"

#include <imgui.h>
#include <spdlog/spdlog.h>

#include "IconsFontAwesome6.h"
#include "assets/ModelAsset.h"
#include "assets/materialAsset/MaterialAsset.hpp"
#include "assets/shaderAsset/ShaderAsset.h"
#include "assets/shaderProgram/ShaderProgramAsset.h"
#include "assets/engineAssets/SceneAsset.h"
#include "assets/engineAssets/PrefabAsset.h"
#include "assets/configAsset/ConfigAsset.h"
#include "assets/meshAsset/MeshAsset.h"
#include "assets/textureAsset/TextureAsset.h"
#include "JsonHelpers.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#endif

namespace am {

    void AssetManager::Initialize(plt::PlatformInterface* platformInterface)
    {
        this->platform = platformInterface;
        platformInterface->SubscribeToEvent(plt::EventType::FileAddedToFolder,
                   [this](const void* data) {
                       const auto* event = static_cast<const plt::FileAddedEvent*>(data);
                       this->handleFileAddedToFolder(event);
                   });
        platformInterface->SubscribeToEvent(plt::EventType::FileDropped,
                 [this](const void* data) {
                     const auto* event = static_cast<const plt::FileDropEvent*>(data);
                     this->handleFileDropped(event);
                 });
    }

    AssetManager &AssetManager::getInstance() {
        static AssetManager instance;
        return instance;
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

        std::string normalizedPath = (p.parent_path() / lookUpName).string();

        return initializeAsset(assetType, normalizedPath, lookUpName);
    }

    std::optional<boost::uuids::uuid> AssetManager::createAsset(AssetType assetType, std::string path, std::string lookupName) {
        std::filesystem::path p = std::filesystem::path(path).lexically_normal();

        if (lookupNamesToUUIDs.find(lookupName) != lookupNamesToUUIDs.end())
        {
            spdlog::error("Lookup name already exists");
            throw std::runtime_error("Lookup name already exists");
        }

        std::string normalizedPath = (p.parent_path() / (lookupName + GetExtensionFromAssetType(assetType))).string();

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
            auto newAsset = creator(id, path);
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
            saveAssetMetadata(id);
            return id;
        }
        catch (std::exception& e)
        {
            spdlog::error("Failed to create asset");
        }
        return boost::uuids::nil_uuid();
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

std::optional<std::shared_ptr<AssetInfo> > AssetManager::getAssetInfo(const boost::uuids::uuid &id) const {
    auto it = metadata.find(id);
    if (it != metadata.end()) return it->second;
    spdlog::error("No asset found with id: {}", boost::uuids::to_string(id));
    return std::nullopt;
}

    std::optional<Asset*> AssetManager::getAsset(const boost::uuids::uuid& id)
    {
        auto it = assets.find(id);
        if (it != assets.end()) return it->second.get();

        auto assetInfo = metadata.find(id);
        if (assetInfo == metadata.end()) return std::nullopt;
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
                assetNew = binLoader(assetInfo->second->path, AssetFormat::Binary);
            }else
            {
                auto jsonLoader = getLoader(getTypeIndex(assetInfo->second->type));
                if (!jsonLoader) {
                    spdlog::error("No factory registered for asset type");
                    throw std::runtime_error("No factory registered for asset type");
                }
                assetNew = jsonLoader(assetInfo->second->path, AssetFormat::Json);
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
    
    void AssetManager::saveAsset(const boost::uuids::uuid id) //TODO prob i should also save the depended assets
    {
        if (id.is_nil())
        {
            spdlog::error("Cannot save asset with nil id");
            return;
        }
        auto info = getAssetInfo(id);
        if (!info)
        {
            spdlog::error("No asset info found with id: {}", boost::uuids::to_string(id));
            return;
        }
        auto asset = getAsset(id);
        if (!asset)
        {
            spdlog::error("No asset found with id: {}", boost::uuids::to_string(id));
            return;
        }

        if (GetEditorSavesToBin(info.value()->type))
        {
            asset.value()->SaveAssetToBin(info.value()->path);
        }else
        {
            rapidjson::Document document;
            document.SetObject();

            asset.value()->SaveAssetToJson(document);

            auto& allocator = document.GetAllocator();

            // Add encoding information if not present
            if (!document.HasMember("_meta")) {
                rapidjson::Value encodingInfo(rapidjson::kObjectType);
                encodingInfo.AddMember("encoding", "UTF-8", allocator);
                encodingInfo.AddMember("version", "1.0", allocator);
                document.AddMember("_meta", encodingInfo, allocator);
            }

            saveJsonToFile(info.value()->path, document);
        }
        saveAssetMetadata(id);
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

    void AssetManager::scanResourceDirectory(const std::filesystem::path& rootPath)
    {
        if (!std::filesystem::exists(rootPath)) {
            spdlog::warn("Resource folder does not exist: {}", rootPath.string());
            return;
        }

        resourceFolder = rootPath.lexically_normal().string();

        metadata.clear();
        lookupNamesToUUIDs.clear();
        assets.clear();

        // 1. Attempt to load from metadata cache for fast startup
        std::filesystem::path cacheFile = std::filesystem::path(resourceFolder) / ".cache" / "metadata_cache.json";
        bool cacheLoaded = loadMetaCache(cacheFile.string());

        // 2. Scan all .meta sidecars to discover newly added or modified assets
        for (const auto& entry : std::filesystem::recursive_directory_iterator(rootPath))
        {
            if (!entry.is_regular_file()) continue;

            const auto& path = entry.path();
            if (path.extension() == ".meta")
            {
                rapidjson::Document doc;
                if (loadJsonFromFile(path.string(), doc))
                {
                    if (doc.IsObject() && doc.HasMember("id") && doc.HasMember("type"))
                    {
                        auto assetInfo = AssetInfo::DeserializeAssetInfoFromJson(doc);

                        std::string metaPathStr = path.lexically_normal().string();
                        std::string actualAssetPath = metaPathStr.substr(0, metaPathStr.length() - 5);

                        std::error_code ec;
                        if (std::filesystem::exists(actualAssetPath, ec)) {
                            assetInfo.path = actualAssetPath;
                        } else if (assetInfo.path.empty() || !std::filesystem::exists(assetInfo.path, ec)) {
                            assetInfo.path = actualAssetPath;
                        } else {
                            assetInfo.path = std::filesystem::path(assetInfo.path).lexically_normal().string();
                        }

                        auto infoPtr = std::make_shared<AssetInfo>(std::move(assetInfo));
                        metadata[infoPtr->id] = infoPtr;
                        lookupNamesToUUIDs[infoPtr->lookUpName] = infoPtr->id;
                    }
                }
            }
        }

        // 3. Save updated metadata cache
        saveMetaCache(cacheFile.string());

        spdlog::info("Scanned resource directory '{}', loaded {} assets (cache: {}).", rootPath.string(), metadata.size(), cacheLoaded ? "loaded" : "built");
    }

    bool AssetManager::saveAssetMetadata(const boost::uuids::uuid& assetId) const
    {
        auto it = metadata.find(assetId);
        if (it == metadata.end()) {
            spdlog::error("Cannot save metadata: no asset found with id {}", boost::uuids::to_string(assetId));
            return false;
        }

        std::string metaPath = it->second->path + ".meta";
        rapidjson::Document document;
        document.SetObject();
        auto& allocator = document.GetAllocator();

        // Add encoding information
        rapidjson::Value encodingInfo(rapidjson::kObjectType);
        encodingInfo.AddMember("encoding", "UTF-8", allocator);
        encodingInfo.AddMember("version", "1.0", allocator);
        document.AddMember("_meta", encodingInfo, allocator);

        it->second->SerializeAssetInfoToJson(document, allocator);

        bool saved = saveJsonToFile(metaPath, document);
        if (saved) {
            saveMetaCache();
        }
        return saved;
    }

    void AssetManager::saveAllAssetMetadata() const
    {
        for (const auto& [id, info] : metadata)
        {
            saveAssetMetadata(id);
        }
        saveMetaCache();
    }

    bool AssetManager::saveMetaCache(const std::string& cachePath) const
    {
        std::filesystem::path targetPath = cachePath.empty()
            ? (std::filesystem::path(resourceFolder) / ".cache" / "metadata_cache.json")
            : std::filesystem::path(cachePath);

        std::error_code ec;
        std::filesystem::create_directories(targetPath.parent_path(), ec);

        rapidjson::Document document;
        document.SetObject();
        auto& allocator = document.GetAllocator();

        rapidjson::Value encodingInfo(rapidjson::kObjectType);
        encodingInfo.AddMember("encoding", "UTF-8", allocator);
        encodingInfo.AddMember("version", "1.0", allocator);
        document.AddMember("_meta", encodingInfo, allocator);

        rapidjson::Value metadataArray(rapidjson::kArrayType);
        for (const auto& [uuid, info] : metadata) {
            rapidjson::Value assetInfoObj(rapidjson::kObjectType);
            info->SerializeAssetInfoToJson(assetInfoObj, allocator);
            metadataArray.PushBack(assetInfoObj, allocator);
        }
        document.AddMember("metadata", metadataArray, allocator);

        return saveJsonToFile(targetPath.string(), document);
    }

    bool AssetManager::loadMetaCache(const std::string& cachePath)
    {
        std::filesystem::path targetPath = cachePath.empty()
            ? (std::filesystem::path(resourceFolder) / ".cache" / "metadata_cache.json")
            : std::filesystem::path(cachePath);

        if (!std::filesystem::exists(targetPath)) {
            return false;
        }

        rapidjson::Document doc;
        if (!loadJsonFromFile(targetPath.string(), doc) || !doc.IsObject() || !doc.HasMember("metadata")) {
            return false;
        }

        const auto& array = doc["metadata"];
        if (!array.IsArray()) {
            return false;
        }

        for (rapidjson::SizeType i = 0; i < array.Size(); i++) {
            const auto& obj = array[i];
            if (obj.IsObject() && obj.HasMember("id") && obj.HasMember("type")) {
                auto assetInfo = AssetInfo::DeserializeAssetInfoFromJson(obj);
                std::string actualAssetPath = assetInfo.path;
                std::error_code ec;
                if (!std::filesystem::exists(actualAssetPath, ec)) {
                    continue;
                }
                auto infoPtr = std::make_shared<AssetInfo>(std::move(assetInfo));
                metadata[infoPtr->id] = infoPtr;
                lookupNamesToUUIDs[infoPtr->lookUpName] = infoPtr->id;
            }
        }
        return !metadata.empty();
    }

    std::string AssetManager::getThumbnailPath(const boost::uuids::uuid& id) const
    {
        auto it = metadata.find(id);
        if (it != metadata.end() && !it->second->thumbnailPath.empty()) {
            std::filesystem::path p = it->second->thumbnailPath;
            if (p.is_relative()) {
                std::filesystem::path fullPath = std::filesystem::path(resourceFolder) / p;
                if (std::filesystem::exists(fullPath)) {
                    return fullPath.string();
                }
            }
            if (std::filesystem::exists(p)) {
                return p.string();
            }
        }

        std::filesystem::path defaultCachePath = std::filesystem::path(resourceFolder) / ".cache" / "thumbnails" / (boost::uuids::to_string(id) + ".png");
        if (std::filesystem::exists(defaultCachePath)) {
            return defaultCachePath.string();
        }

        return "";
    }

    bool AssetManager::generateThumbnail(const boost::uuids::uuid& id)
    {
        auto it = metadata.find(id);
        if (it == metadata.end()) {
            return false;
        }

        auto& info = it->second;
        if (info->type == AssetType::Texture) {
            std::filesystem::path thumbDir = std::filesystem::path(resourceFolder) / ".cache" / "thumbnails";
            std::error_code ec;
            std::filesystem::create_directories(thumbDir, ec);

            std::filesystem::path destPath = thumbDir / (boost::uuids::to_string(id) + ".png");

            int thumbW = 128;
            int thumbH = 128;
            std::vector<uint8_t> thumbPixels(thumbW * thumbH * 4);

            // 1. Try loading from original source image (e.g. .png, .jpg) via importPath or info->path
            std::filesystem::path importPath = info->importContext.importPath;
            if (!std::filesystem::exists(importPath, ec)) {
                std::filesystem::path p1 = std::filesystem::path(resourceFolder) / importPath;
                if (std::filesystem::exists(p1, ec)) {
                    importPath = p1;
                } else if (importPath.string().rfind("res/", 0) == 0 || importPath.string().rfind("res\\", 0) == 0) {
                    std::filesystem::path subPath = importPath.string().substr(4);
                    std::filesystem::path p2 = std::filesystem::path(resourceFolder) / subPath;
                    if (std::filesystem::exists(p2, ec)) {
                        importPath = p2;
                    }
                }
            }

            if (!std::filesystem::exists(importPath, ec) || importPath.extension() == ".b_texture") {
                std::filesystem::path infoP = info->path;
                if (std::filesystem::exists(infoP, ec) && infoP.extension() != ".b_texture") {
                    importPath = infoP;
                } else {
                    std::filesystem::path p1 = std::filesystem::path(resourceFolder) / infoP;
                    if (std::filesystem::exists(p1, ec) && p1.extension() != ".b_texture") {
                        importPath = p1;
                    }
                }
            }

            if (std::filesystem::exists(importPath, ec) && importPath.extension() != ".b_texture") {
                stbi_set_flip_vertically_on_load(false);
                int width = 0, height = 0, channels = 0;
                unsigned char* pixels = stbi_load(importPath.string().c_str(), &width, &height, &channels, 4);
                if (pixels) {
                    stbir_resize_uint8_linear(pixels, width, height, 0, thumbPixels.data(), thumbW, thumbH, 0, STBIR_RGBA);
                    stbi_image_free(pixels);

                    if (stbi_write_png(destPath.string().c_str(), thumbW, thumbH, 4, thumbPixels.data(), thumbW * 4)) {
                        info->thumbnailPath = ".cache/thumbnails/" + boost::uuids::to_string(id) + ".png";
                        saveAssetMetadata(id);
                        return true;
                    }
                }
            }

            // 2. If original image is not directly readable via stbi_load (or asset is binary .b_texture), load the TextureAsset
            auto assetOpt = getAsset(id);
            if (assetOpt.has_value() && assetOpt.value()) {
                auto* texData = assetOpt.value()->getAssetDataAs<TextureData>();
                if (texData && !texData->pixels.empty() && texData->width > 0 && texData->height > 0) {
                    stbir_resize_uint8_linear(
                        reinterpret_cast<const unsigned char*>(texData->pixels.data()),
                        texData->width,
                        texData->height,
                        0,
                        thumbPixels.data(),
                        thumbW,
                        thumbH,
                        0,
                        STBIR_RGBA);

                    if (stbi_write_png(destPath.string().c_str(), thumbW, thumbH, 4, thumbPixels.data(), thumbW * 4)) {
                        info->thumbnailPath = ".cache/thumbnails/" + boost::uuids::to_string(id) + ".png";
                        saveAssetMetadata(id);
                        return true;
                    }
                }
            }
        }
        else if (info->type == AssetType::Model || info->type == AssetType::Mesh) {
            std::filesystem::path thumbDir = std::filesystem::path(resourceFolder) / ".cache" / "thumbnails";
            std::error_code ec;
            std::filesystem::create_directories(thumbDir, ec);

            std::filesystem::path destPath = thumbDir / (boost::uuids::to_string(id) + ".png");

            if (engine) {
                if (engine->CaptureModelThumbnail(id, destPath.string())) {
                    info->thumbnailPath = ".cache/thumbnails/" + boost::uuids::to_string(id) + ".png";
                    saveAssetMetadata(id);
                    return true;
                }
            }
        }
        return false;
    }

    void* AssetManager::getThumbnailTexture(const boost::uuids::uuid& id)
    {
        if (!engine) {
            return nullptr;
        }

        std::string thumbPath = getThumbnailPath(id);
        if (thumbPath.empty() || !std::filesystem::exists(thumbPath)) {
            if (generateThumbnail(id)) {
                thumbPath = getThumbnailPath(id);
            }
        }

        if (!thumbPath.empty() && std::filesystem::exists(thumbPath)) {
            return engine->GetThumbnailTexture(id, thumbPath);
        }

        return nullptr;
    }

    void* AssetManager::getThumbnailTexture(const std::filesystem::path& path)
    {
        auto uuidOpt = getAssetUuidByPath(path);
        if (uuidOpt) {
            return getThumbnailTexture(uuidOpt.value());
        }
        return nullptr;
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


    AssetManager::AssetManager() : AssetManagerInterface()
    {
        if (std::filesystem::exists("res")) {
            resourceFolder = std::filesystem::absolute("res").lexically_normal().string();
        } else if (std::filesystem::exists("C:\\Users\\redkc\\CLionProjects\\ReasonableVulkanPublic\\res")) {
            resourceFolder = "C:\\Users\\redkc\\CLionProjects\\ReasonableVulkanPublic\\res";
        }
        currentPath = resourceFolder;

        // Auto-register all supported engine assets at compile time using reflection
        ForEachType<SupportedAssets>([this]<typename T>() {
            this->RegisterAssetType<T>();
        });

        scanResourceDirectory(resourceFolder);
        loadFileBrowserConfig();
    }

    AssetManager::~AssetManager() {
        saveFileBrowserConfig();
        saveAllAssetMetadata();
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

    void AssetManager::handleFileAddedToFolder(const plt::FileAddedEvent* event)
    {
        auto filePath = event->filePath;
        auto ext = std::filesystem::path(filePath).extension().string();

        if (ext == ".meta")
        {
            rapidjson::Document doc;
            if (loadJsonFromFile(filePath, doc))
            {
                if (doc.IsObject() && doc.HasMember("id") && doc.HasMember("type"))
                {
                    auto assetInfo = AssetInfo::DeserializeAssetInfoFromJson(doc);
                    std::string metaPathStr = std::filesystem::path(filePath).lexically_normal().string();
                    std::string actualAssetPath = metaPathStr.substr(0, metaPathStr.length() - 5);
                    std::error_code ec;
                    if (std::filesystem::exists(actualAssetPath, ec)) {
                        assetInfo.path = actualAssetPath;
                    }
                    auto infoPtr = std::make_shared<AssetInfo>(std::move(assetInfo));
                    metadata[infoPtr->id] = infoPtr;
                    lookupNamesToUUIDs[infoPtr->lookUpName] = infoPtr->id;
                }
            }
            return;
        }

        auto assetOwnership = StringToAssetOwnership(ext);

        if (assetOwnership == AssetOwnership::Managed)
        {
            AssetType assetType = GetAssetTypeFromExtension(ext);
            unique_ptr<Asset> assetNew;
            try
            {
                if (GetEditorSavesToBin(assetType))
                {
                    auto binLoader = getLoader(getTypeIndex(assetType));
                    if (!binLoader) {
                        spdlog::error("No factory registered for asset type");
                        throw std::runtime_error("No factory registered for asset type");
                    }
                    assetNew = binLoader(event->filePath, AssetFormat::Binary);
                }else
                {
                    auto jsonLoader = getLoader(getTypeIndex(assetType));
                    if (!jsonLoader) {
                        spdlog::error("No factory registered for asset type");
                        throw std::runtime_error("No factory registered for asset type");
                    }
                    assetNew = jsonLoader(event->filePath, AssetFormat::Json);
                }
                auto id = assetNew->id;
                auto assetInfoIt = metadata.find(id);
                if (assetInfoIt != metadata.end()) {
                    if (std::filesystem::exists(assetInfoIt->second->path) && std::filesystem::path(assetInfoIt->second->path) != std::filesystem::path(event->filePath)) {
                        std::filesystem::remove(assetInfoIt->second->path);
                    }
                    assetInfoIt->second->path = event->filePath;
                    assetInfoIt->second->loadedAsset = assetNew.get();
                    assetInfoIt->second->isLoaded = true;
                }
                assets[id] = std::move(assetNew);
            }catch (const std::exception& e)
            {
                spdlog::error("Failed to load asset: {}", e.what());
            }
        }
        //Don't do anything for other types
    }

    void AssetManager::handleFileDropped(const plt::FileDropEvent* event)
    {
        auto filePath = std::filesystem::path(event->filePath).lexically_normal();
        auto ext = filePath.extension().string();
        auto assetOwnership = StringToAssetOwnership(ext);

        if (assetOwnership == AssetOwnership::Import)
        {
            // If it's outside the current path, copy it there
            std::filesystem::path destPath = currentPath / filePath.filename();

            if (filePath != destPath)
            {
                try {
                    std::filesystem::copy(filePath, destPath, std::filesystem::copy_options::overwrite_existing);
                    registerAsset(destPath.string());
                } catch (const std::exception& e) {
                    spdlog::error("Failed to copy dropped file: {}", e.what());
                }
            } else {
                registerAsset(filePath.string());
            }
        }
        //Don't do anything for other types
    }


    void AssetManager::setEngine(engine::EngineInterface* engineInterface)
    {
        this->engine = engineInterface;
    }

    std::optional<boost::uuids::uuid> AssetManager::getAssetUuidByPath(const std::filesystem::path& path)
    {
        std::filesystem::path normalPath = path.lexically_normal();

        // 1. Direct match on info->path
        for (const auto& [id, info] : metadata)
        {
            if (std::filesystem::path(info->path).lexically_normal() == normalPath)
            {
                return id;
            }
        }

        // 2. Direct match or relative match on info->importContext.importPath
        for (const auto& [id, info] : metadata)
        {
            if (!info->importContext.importPath.empty())
            {
                std::filesystem::path impPath = std::filesystem::path(info->importContext.importPath).lexically_normal();
                if (impPath == normalPath)
                {
                    return id;
                }
                std::filesystem::path relImpPath = (std::filesystem::path(resourceFolder) / info->importContext.importPath).lexically_normal();
                if (relImpPath == normalPath)
                {
                    return id;
                }
                if (info->importContext.importPath.rfind("res/", 0) == 0 || info->importContext.importPath.rfind("res\\", 0) == 0) {
                    std::filesystem::path subImp = (std::filesystem::path(resourceFolder) / info->importContext.importPath.substr(4)).lexically_normal();
                    if (subImp == normalPath) {
                        return id;
                    }
                }
            }
        }

        // 3. Equivalent path on info->path or importPath
        for (const auto& [id, info] : metadata)
        {
            std::error_code ec;
            if (std::filesystem::exists(info->path, ec) && std::filesystem::exists(normalPath, ec))
            {
                if (std::filesystem::equivalent(info->path, normalPath, ec))
                {
                    return id;
                }
            }
            if (!info->importContext.importPath.empty())
            {
                if (std::filesystem::exists(info->importContext.importPath, ec) && std::filesystem::exists(normalPath, ec))
                {
                    if (std::filesystem::equivalent(info->importContext.importPath, normalPath, ec))
                    {
                        return id;
                    }
                }
                std::filesystem::path relImpPath = std::filesystem::path(resourceFolder) / info->importContext.importPath;
                if (std::filesystem::exists(relImpPath, ec) && std::filesystem::exists(normalPath, ec))
                {
                    if (std::filesystem::equivalent(relImpPath, normalPath, ec))
                    {
                        return id;
                    }
                }
            }
        }

        // 4. Match by lookup name or stem
        std::string lookUpName = normalPath.filename().string();
        auto it = lookupNamesToUUIDs.find(lookUpName);
        if (it != lookupNamesToUUIDs.end())
        {
            return it->second;
        }

        std::string stemName = normalPath.stem().string();
        it = lookupNamesToUUIDs.find(stemName);
        if (it != lookupNamesToUUIDs.end())
        {
            return it->second;
        }

        // 5. Match by stem and parent path against metadata
        for (const auto& [id, info] : metadata)
        {
            std::filesystem::path infoP = std::filesystem::path(info->path).lexically_normal();
            if (infoP.stem() == normalPath.stem() && infoP.parent_path() == normalPath.parent_path())
            {
                return id;
            }
            if (!info->importContext.importPath.empty()) {
                std::filesystem::path impP = std::filesystem::path(info->importContext.importPath).lexically_normal();
                if (impP.stem() == normalPath.stem() && impP.parent_path() == normalPath.parent_path())
                {
                    return id;
                }
            }
        }

        if (std::filesystem::exists(normalPath))
        {
            std::filesystem::path metaPath = normalPath.string() + ".meta";
            if (std::filesystem::exists(metaPath)) {
                rapidjson::Document doc;
                if (loadJsonFromFile(metaPath.string(), doc)) {
                    if (doc.IsObject() && doc.HasMember("id") && doc.HasMember("type")) {
                        auto assetInfo = AssetInfo::DeserializeAssetInfoFromJson(doc);
                        assetInfo.path = normalPath.string();
                        auto infoPtr = std::make_shared<AssetInfo>(std::move(assetInfo));
                        metadata[infoPtr->id] = infoPtr;
                        lookupNamesToUUIDs[infoPtr->lookUpName] = infoPtr->id;
                        return infoPtr->id;
                    }
                }
            }

            std::filesystem::path bTexMeta = normalPath.parent_path() / (normalPath.stem().string() + ".b_texture.meta");
            if (std::filesystem::exists(bTexMeta)) {
                rapidjson::Document doc;
                if (loadJsonFromFile(bTexMeta.string(), doc)) {
                    if (doc.IsObject() && doc.HasMember("id") && doc.HasMember("type")) {
                        auto assetInfo = AssetInfo::DeserializeAssetInfoFromJson(doc);
                        auto infoPtr = std::make_shared<AssetInfo>(std::move(assetInfo));
                        metadata[infoPtr->id] = infoPtr;
                        lookupNamesToUUIDs[infoPtr->lookUpName] = infoPtr->id;
                        return infoPtr->id;
                    }
                }
            }

            auto ext = normalPath.extension().string();
            AssetType assetType = GetAssetTypeFromExtension(ext);
            if (assetType == AssetType::Scene)
            {
                rapidjson::Document doc;
                if (loadJsonFromFile(normalPath.string(), doc))
                {
                    if (doc.HasMember("uuid") && doc["uuid"].IsString())
                    {
                        boost::uuids::uuid fileId = boost::uuids::string_generator()(doc["uuid"].GetString());
                        auto assetInfo = std::make_shared<AssetInfo>(fileId, normalPath.string(), AssetType::Scene, 0, ImportContext(normalPath.string(), AssetType::Scene, 0), stemName);
                        metadata[fileId] = assetInfo;
                        lookupNamesToUUIDs[stemName] = fileId;
                        saveAssetMetadata(fileId);
                        return fileId;
                    }
                }
            }
        }

        return std::nullopt;
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

    void AssetManager::openAssetFile(const std::filesystem::path& path)
    {
        AssetType type = GetAssetTypeFromExtension(path.extension().string());
        if (type == AssetType::Scene && engine) {
            auto sceneUuid = getAssetUuidByPath(path);
            if (sceneUuid) {
                engine->LoadScene(sceneUuid.value());
                return;
            }
        } else if ((type == AssetType::Model || type == AssetType::Mesh) && engine) {
            auto modelUuid = getAssetUuidByPath(path);
            if (modelUuid) {
                engine->OpenModelPreviewScene(modelUuid.value());
                return;
            }
        }

        if (openFileInIDE(path)) {
            return;
        }

        openFileWithDefaultApp(path);
    }

    bool AssetManager::openFileInIDE(const std::filesystem::path& path)
    {
        if (path.empty()) return false;
        std::filesystem::path absPath = std::filesystem::absolute(path);

        if (platform) {
            return platform->OpenFileInIDE(absPath.string());
        }
        return false;
    }

    bool AssetManager::openFileWithDefaultApp(const std::filesystem::path& path)
    {
        if (path.empty()) return false;
        std::filesystem::path absPath = std::filesystem::absolute(path);

        if (platform) {
            return platform->OpenFileInDefaultApp(absPath.string());
        }

#ifdef _WIN32
        HINSTANCE result = ShellExecuteW(NULL, L"open", absPath.wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
        return reinterpret_cast<intptr_t>(result) > 32;
#elif defined(__APPLE__)
        std::string cmd = "open \"" + absPath.string() + "\"";
        return system(cmd.c_str()) == 0;
#else
        std::string cmd = "xdg-open \"" + absPath.string() + "\"";
        return system(cmd.c_str()) == 0;
#endif
    }


    std::optional<boost::uuids::uuid> AssetManager::importAsset(ImportContext importContext, std::string lookUpName)
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

            size_t contentHash = newAsset->calculateContentHash(); // We don't check for duplicates. Maby do that on realise
            /*
            // Check if we have an asset with the same content hash
            auto existingAsset = std::find_if(metadata.begin(), metadata.end(),
                                              [contentHash](const auto &pair) {
                                                  return pair.second->contentHash == contentHash;
                                              });

            if (existingAsset != metadata.end()) {
                // We found an asset with the same content
                return existingAsset->second.get()->id;
            }

            */

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
            saveAssetMetadata(id);
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
        std::optional<std::type_index> result;
        ForEachType<SupportedAssets>([&]<typename T>() {
            if (T::StaticType == type) {
                result = std::type_index(typeid(T));
            }
        });
        if (result.has_value()) {
            return result.value();
        }
        spdlog::error("Failed to get type_index for AssetType");
        throw std::runtime_error("Failed to get type_index for AssetType!");
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

    bool AssetManager::reimportAsset(const boost::uuids::uuid& id)
    {
        auto it = metadata.find(id);
        if (it == metadata.end()) {
            spdlog::error("Cannot reimport asset: UUID not found in metadata");
            return false;
        }

        auto info = it->second;
        ImportContext context = info->importContext;

        if (context.importPath.empty()) {
            context.importPath = info->path;
        }
        if (context.assetType == AssetType::Other) {
            context.assetType = info->type;
        }

        std::filesystem::path importP(context.importPath);
        std::error_code ec;
        if (!std::filesystem::exists(importP, ec)) {
            if (std::filesystem::exists(std::filesystem::path(resourceFolder) / importP, ec)) {
                importP = std::filesystem::path(resourceFolder) / importP;
            }
        }
        context.importPath = importP.lexically_normal().string();

        try
        {
            auto factory = getImporter(getTypeIndex(context.assetType));
            if (!factory) {
                spdlog::error("No importer factory registered for asset type {}", static_cast<int>(context.assetType));
                return false;
            }

            std::unique_ptr<Asset> newAsset = factory(id, context);
            if (!newAsset) {
                spdlog::error("Failed to reimport asset {}", boost::uuids::to_string(id));
                return false;
            }

            size_t contentHash = newAsset->calculateContentHash();
            info->contentHash = contentHash;
            info->isLoaded = true;
            info->importContext = context;

            std::filesystem::path p = std::filesystem::path(context.importPath).lexically_normal();
            std::string baseName = p.stem().string();

            if (GetEditorSavesToBin(context.assetType))
            {
                std::string additionalSufix = "";
                if (info->type == AssetType::Shader)
                {
                    additionalSufix = GetShaderSufix(newAsset->getAssetDataAs<ShaderData>()->stage);
                }
                std::string filename = info->path.empty() ? GetBinPath((p.parent_path() / (baseName + GetExtensionFromAssetType(context.assetType))).string(), additionalSufix) : info->path;
                info->path = filename;
                newAsset->SaveAssetToBin(filename);
            } else {
                std::string filename = info->path.empty() ? (p.parent_path() / (baseName + GetExtensionFromAssetType(context.assetType))).string() : info->path;
                info->path = filename;
                auto jsonSaver = getJsonSaver(getTypeIndex(context.assetType));
                if (jsonSaver) {
                    rapidjson::Document document;
                    document.SetObject();
                    auto& allocator = document.GetAllocator();

                    rapidjson::Value encodingInfo(rapidjson::kObjectType);
                    encodingInfo.AddMember("encoding", "UTF-8", allocator);
                    encodingInfo.AddMember("version", "1.0", allocator);
                    document.AddMember("_meta", encodingInfo, allocator);

                    jsonSaver(*newAsset, document);
                    saveJsonToFile(filename, document);
                }
            }

            assets[id] = std::move(newAsset);
            info->loadedAsset = assets[id].get();
            saveAssetMetadata(id);
            generateThumbnail(id);
            spdlog::info("Successfully reimported asset {} from {}", boost::uuids::to_string(id), context.importPath);
            return true;
        }
        catch (const std::exception& e)
        {
            spdlog::error("Failed to reimport asset {}: {}", boost::uuids::to_string(id), e.what());
            return false;
        }
    }

    bool AssetManager::reimportAsset(const std::filesystem::path& path)
    {
        auto uuidOpt = getAssetUuidByPath(path);
        if (uuidOpt) {
            return reimportAsset(uuidOpt.value());
        }

        auto regOpt = registerAsset(path.string());
        return regOpt.has_value();
    }

}
