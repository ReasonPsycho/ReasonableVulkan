//
// Created by redkc on 09.05.2025.
//

#include "../include/Asset.hpp"
#include "AssetManager.hpp"

#include <imgui.h>
#include <spdlog/spdlog.h>

#include "assets/ModelAsset.h"
#include "assets/materialAsset/MaterialAsset.hpp"
#include "assets/shaderAsset/ShaderAsset.h"
#include "assets/shaderProgram/ShaderProgramAsset.h"
#include "assets/engineAssets/SceneAsset.h"
#include "assets/engineAssets/PrefabAsset.h"
#include "assets/configAsset/ConfigAsset.h"
#include "JsonHelpers.hpp"

namespace am {

    void AssetManager::Initialize(plt::PlatformInterface* platformInterface)
    {
        /*
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->AddFontDefaultVector();
        ImFontConfig config;
        config.MergeMode = true;
        config.GlyphMinAdvanceX = 13.0f; // Use if you want to make the icon monospaced
        io.Fonts->AddFontFromFileTTF("fonts/fontawesome-webfont.ttf", 13.0f, &config);
        */


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

        std::string normalizedPath = p.parent_path().string() + "/" + lookUpName;

        return initializeAsset(assetType, normalizedPath, lookUpName);
    }

    std::optional<boost::uuids::uuid> AssetManager::createAsset(AssetType assetType, std::string path, std::string lookupName) {
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
    spdlog::error("No asset found!");
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
        currentPath = resourceFolder;
        RegisterAssetType<MaterialAsset>();
        RegisterAssetType<TextureAsset>();
        RegisterAssetType<ShaderAsset>();
        RegisterAssetType<ShaderProgramAsset>();
        RegisterAssetType<ModelAsset>();
        RegisterAssetType<MeshAsset>();
        RegisterAssetType<SceneAsset>();
        RegisterAssetType<PrefabAsset>();
        RegisterAssetType<ConfigAsset>();

        loadRegistryMetadataFromFile("C:\\Users\\redkc\\CLionProjects\\ReasonableVulkan\\res\\metadatas.json");
    }

    AssetManager::~AssetManager() {
        saveRegistryMetadataToFile("C:\\Users\\redkc\\CLionProjects\\ReasonableVulkan\\res\\metadatas.json");
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

    void AssetManager::ImguiFileBrowser(std::string windowName)
    {
        ImGui::Begin(windowName.c_str());

        if (currentPath != resourceFolder)
        {
            if (ImGui::Button(".."))
            {
                currentPath = currentPath.parent_path();
            }
            ImGui::SameLine();
        }
        ImGui::Text("Current Path: %s", currentPath.string().c_str());

        ImGui::Separator();

        if (ImGui::BeginChild("FileBrowserScroll"))
        {
            float iconSize = 64.0f;
            float padding = 16.0f;
            float cellSize = iconSize + padding;

            float windowVisibleX2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

            try {
                int i = 0;
                for (const auto& entry : std::filesystem::directory_iterator(currentPath))
                {
                    const auto& path = entry.path();
                    std::string filename = path.filename().string();

                    ImGui::PushID(i++);
                    ImGui::BeginGroup();

                    // Dummy "icon"
                    if (entry.is_directory()) {
                        ImGui::Button("[DIR]", ImVec2(iconSize, iconSize));
                        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                            currentPath = path;
                        }
                    } else {
                        ImGui::Button("[FILE]", ImVec2(iconSize, iconSize));
                    }

                    // Centered text below icon
                    float textWidth = ImGui::CalcTextSize(filename.c_str()).x;
                    if (textWidth > iconSize) {
                        // Truncate text if too long
                        std::string truncated = filename.substr(0, 8) + "...";
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (iconSize - ImGui::CalcTextSize(truncated.c_str()).x) * 0.5f);
                        ImGui::Text("%s", truncated.c_str());
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", filename.c_str());
                    } else {
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (iconSize - textWidth) * 0.5f);
                        ImGui::Text("%s", filename.c_str());
                    }

                    ImGui::EndGroup();

                    float lastButtonX2 = ImGui::GetItemRectMax().x;
                    float nextButtonX2 = lastButtonX2 + padding + cellSize;
                    if (nextButtonX2 < windowVisibleX2)
                        ImGui::SameLine(0, padding);

                    ImGui::PopID();
                }
            } catch (const std::exception& e) {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: %s", e.what());
                if (ImGui::Button("Reset to Resource Folder")) {
                    currentPath = resourceFolder;
                }
            }
            ImGui::EndChild();
        }

        ImGui::End();
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
        case AssetType::Config:
            return std::type_index(typeid(ConfigAsset));
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
