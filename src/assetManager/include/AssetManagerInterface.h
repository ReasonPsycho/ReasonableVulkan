//
// Created by redkc on 07/08/2025.
//

#ifndef ASSETMANAGERINTERFACE_H
#define ASSETMANAGERINTERFACE_H
#include <any>
#include <filesystem>
#include <boost/uuid/uuid.hpp>
#include "AssetInfo.hpp"
#include "PlatformInterface.hpp"

namespace engine {
    class EngineInterface;
}

namespace am
{
    class AssetManagerInterface{
    public:
        virtual void Initialize(plt::PlatformInterface* platformInterface) = 0;
        virtual ~AssetManagerInterface() = default;

        virtual void setEngine(engine::EngineInterface* engineInterface) = 0;
        virtual std::optional<boost::uuids::uuid> getAssetUuidByPath(const std::filesystem::path& path) = 0;

        virtual std::optional<boost::uuids::uuid> createAsset(AssetType assetType, std::string path) = 0;
        virtual std::optional<boost::uuids::uuid> createAsset(AssetType assetType, std::string path, std::string lookUpName) = 0;

        virtual std::optional<boost::uuids::uuid> registerAsset(std::string path) = 0;
        virtual std::optional<boost::uuids::uuid> registerAsset(std::string path,std::string lookUpName) = 0;

        virtual std::optional<boost::uuids::uuid> getAssetUuid(std::string lookupName) = 0;

        virtual std::any getAssetData(const boost::uuids::uuid& id) = 0;

        template<typename T>
        T* getAssetData(const boost::uuids::uuid& id) {
            std::any data = getAssetData(id);
            if (auto* ptr = std::any_cast<T*>(&data)) {
                return *ptr;
            }
            return nullptr;
        }

        virtual std::any getAssetData(std::string lookupName) = 0;

        template<typename T>
        T* getAssetData(std::string lookupName) {
            std::any data = getAssetData(lookupName);
            if (auto* ptr = std::any_cast<T*>(&data)) {
                return *ptr;
            }
            return nullptr;
        }

        virtual std::optional<std::shared_ptr<AssetInfo>> getAssetInfo(const boost::uuids::uuid& id) const = 0;
        virtual std::optional<Asset*> getAsset(const boost::uuids::uuid& id) = 0;

        virtual void saveAsset(boost::uuids::uuid id) = 0;
        virtual void saveAsset(std::string lookupName) = 0;

        virtual std::string getThumbnailPath(const boost::uuids::uuid& id) const = 0;
        virtual bool generateThumbnail(const boost::uuids::uuid& id) = 0;
        virtual void* getThumbnailTexture(const boost::uuids::uuid& id) = 0;
        virtual void* getThumbnailTexture(const std::filesystem::path& path) = 0;

        virtual bool saveMetaCache(const std::string& cachePath = "") const = 0;
        virtual bool loadMetaCache(const std::string& cachePath = "") = 0;

        virtual std::vector<std::string> getRegisteredAssetsNames() const = 0;
        virtual std::vector<std::string> getRegisteredAssetsNames(AssetType type) const = 0;

        virtual std::vector<boost::uuids::uuid> getRegisteredAssetsUuids() const = 0;
        virtual std::vector<boost::uuids::uuid> getRegisteredAssetsUuids(AssetType type) const = 0;

        virtual void ImguiFileBrowser(std::string windowName) = 0;

        virtual bool openFileWithDefaultApp(const std::filesystem::path& path) = 0;
        virtual bool openFileInIDE(const std::filesystem::path& path) = 0;
        virtual void openAssetFile(const std::filesystem::path& path) = 0;

        virtual bool reimportAsset(const boost::uuids::uuid& id) = 0;
        virtual bool reimportAsset(const std::filesystem::path& path) = 0;
    };

    inline std::shared_ptr<AssetInfo> ResolveAssetInfo(const boost::uuids::uuid& id, AssetManagerInterface* assetManager) {
        if (!assetManager) return nullptr;
        return assetManager->getAssetInfo(id).value_or(nullptr);
    }
}

#endif //ASSETMANAGERINTERFACE_H
