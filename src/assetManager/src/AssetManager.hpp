#ifndef ASSETMANAGER_HPP
#define ASSETMANAGER_HPP

#include "stb_image.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/functional/hash.hpp>
#include "../include/AssetTypes.hpp"
#include "../include/UUIDManager.hpp"
#include <memory>
#include <unordered_map>
#include <functional>
#include <optional>
#include <assimp/Importer.hpp>

#include "AssetManagerInterface.h"
#include "../include/AssetInfo.hpp"

namespace am {
    class AssetManager : public AssetManagerInterface{
        using AssetFactory = std::function<std::unique_ptr<am::Asset>(am::AssetFactoryData &)>;

    public:
        AssetManager(const AssetManager&) = delete;
        AssetManager& operator=(const AssetManager&) = delete;
        static AssetManager &getInstance();

        Assimp::Importer importer;

        //Factories
        AssetFactory getFactory(AssetType type) const;
        void registerFactory(AssetType type, AssetFactory factory) ;

        //Assets
        std::optional<std::shared_ptr<AssetInfo>> registerAsset(std::string path) override;
        std::optional<std::shared_ptr<AssetInfo>> registerAsset(AssetFactoryData *factoryContext);

        [[nodiscard]] std::optional<std::shared_ptr<AssetInfo> > lookupAssetInfo(const boost::uuids::uuid &id) const override;
        [[nodiscard]] std::optional<Asset *> lookupAsset(const boost::uuids::uuid &id) const override;

        //UUIDS
        template <typename T>
             std::shared_ptr<T> getByUUID(const boost::uuids::uuid &id);
        [[nodiscard]] std::vector<boost::uuids::uuid> getUUIDsByPath(const std::string &path) const;

    private:
        AssetManager();
        ~AssetManager();

        std::unordered_map<boost::uuids::uuid, std::unique_ptr<Asset>, boost::hash<boost::uuids::uuid> > assets;
        std::unordered_map<boost::uuids::uuid, std::shared_ptr<AssetInfo>, boost::hash<boost::uuids::uuid> > metadata;
        std::unordered_map<std::string, std::vector<boost::uuids::uuid>> pathToUUIDs;
        std::unordered_map<AssetType, AssetFactory> factories;


    #ifdef AM_ENABLE_TESTS
        friend struct AssetManagerTestFixture;
    #endif
    };


#include "AssetManager.tpp"
} // am

#endif //ASSETMANAGER_HPP