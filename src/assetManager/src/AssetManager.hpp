#ifndef ASSETMANAGER_HPP
#define ASSETMANAGER_HPP

#include <typeindex>
#include "stb_image.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/functional/hash.hpp>
#include "../include/Asset.hpp"
#include "../include/AssetTypes.hpp"
#include "../include/UUIDManager.hpp"
#include <memory>
#include <unordered_map>
#include <functional>
#include <optional>
#include <assimp/Importer.hpp>

#include "AssetManagerInterface.h"
#include "../include/AssetInfo.hpp"


#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>

namespace am {
    class AssetManager : public AssetManagerInterface {
        using AssetCreator = std::function<std::unique_ptr<am::Asset>(const boost::uuids::uuid&)>;
        using AssetImporter = std::function<std::unique_ptr<am::Asset>(const boost::uuids::uuid&, am::ImportContext &)>;
        using AssetJsonSaver = std::function<void(am::Asset&, rapidjson::Document&)>;
        using AssetLoader = std::function<std::unique_ptr<am::Asset>(const std::string&, AssetFormat)>;
        using MetadataLoader = std::function<void(am::Asset&, rapidjson::Document&)>;
        using MetadataSaver  = std::function<void(am::Asset&, rapidjson::Document&)>;

    public:

        void Initialize(plt::PlatformInterface* platformInterface) override;
        AssetManager(const AssetManager&) = delete;
        AssetManager& operator=(const AssetManager&) = delete;
        static AssetManager &getInstance();

        Assimp::Importer importer;

        //Types
        std::type_index getTypeIndex(AssetType type) const;

        //Creators
        AssetImporter getImporter(std::type_index type) const;

        //Importers
        AssetCreator getCreator(std::type_index type) const;

        //Loaders
        AssetLoader getLoader(std::type_index type) const;

        //Jsons
        AssetJsonSaver getJsonSaver(std::type_index type) const;

        //Metadatas
        MetadataLoader getMetadataLoader(std::type_index type) const;
        MetadataSaver getMetadataSaver(std::type_index type) const;


        //Creators
        std::optional<boost::uuids::uuid> createAsset(AssetType assetType, std::string path) override;
        std::optional<boost::uuids::uuid> createAsset(AssetType assetType, std::string path, std::string lookupName) override;

        //Imports
        std::optional<boost::uuids::uuid> registerAsset(std::string path,std::string lookupName) override;
        std::optional<boost::uuids::uuid> registerAsset(ImportContext importContext);
        std::optional<boost::uuids::uuid> registerAsset(std::string path) override;

        //Asset manager inside functions. We should use public ones for importing and creating.
        std::optional<boost::uuids::uuid> importAsset(ImportContext importContext, std::string lookUpName);
        std::optional<boost::uuids::uuid> initializeAsset(AssetType assetType, std::string path, std::string lookupName);

        //Getters
        std::optional<boost::uuids::uuid> getAssetUuid(std::string lookupName) override;

        std::any getAssetData(const boost::uuids::uuid& id) override;
        std::any getAssetData(std::string lookupName) override;

        std::vector<std::string> getRegisteredAssetsNames() const override;
        std::vector<std::string> getRegisteredAssetsNames(AssetType type) const override;

        std::vector<boost::uuids::uuid> getRegisteredAssetsUuids() const override;
        std::vector<boost::uuids::uuid> getRegisteredAssetsUuids(AssetType type) const override;

        void ImguiFileBrowser(std::string windowName) override;
        std::filesystem::path currentPath;

        std::optional<std::shared_ptr<AssetInfo>> getAssetInfo(const boost::uuids::uuid &id) const override;
        std::optional<Asset*> getAsset(const boost::uuids::uuid& id) override;

        void saveAsset(boost::uuids::uuid id) override;
        void saveAsset(std::string lookupName) override;

        //UUIDS
        template <typename T>
        std::shared_ptr<T> getByUUID(const boost::uuids::uuid &id);

        template <typename T>
        void RegisterAssetType();

        //Json
        bool saveRegistryMetadataToFile(const std::string& filename) const;
        bool loadRegistryMetadataFromFile(const std::string& filename);

    private:
        AssetManager();
        ~AssetManager();


        void handleFileAddedToFolder(const plt::FileAddedEvent* event);
        void handleFileDropped(const plt::FileDropEvent* event);

        std::string resourceFolder  = "C:\\Users\\redkc\\CLionProjects\\ReasonableVulkan\\res";
        std::unordered_map<boost::uuids::uuid, std::unique_ptr<Asset>, boost::hash<boost::uuids::uuid>> assets;
        std::unordered_map<boost::uuids::uuid, std::shared_ptr<AssetInfo>, boost::hash<boost::uuids::uuid>> metadata;
        std::unordered_map<std::string, boost::uuids::uuid> lookupNamesToUUIDs;
        std::unordered_map<std::type_index, AssetCreator> creators;
        std::unordered_map<std::type_index, AssetImporter> importers;
        std::unordered_map<std::type_index, AssetJsonSaver> jsonSavers;
        std::unordered_map<std::type_index, AssetLoader> loaders;
        std::unordered_map<std::type_index, MetadataSaver> metadataSavers;
        std::unordered_map<std::type_index, MetadataLoader> metadataLoaders;

    #ifdef AM_ENABLE_TESTS
        friend struct AssetManagerTestFixture;
    #endif
    };

#include "AssetManager.tpp"
} // am

#endif //ASSETMANAGER_HPP