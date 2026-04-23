//
// Created by redkc on 07/08/2025.
//

#ifndef ASSETMANAGERINTERFACE_H
#define ASSETMANAGERINTERFACE_H
#include <any>
#include <boost/uuid/uuid.hpp>
#include <utility>
#include "AssetInfo.hpp"


namespace am
{
    class AssetManagerInterface{
    public:
        virtual ~AssetManagerInterface() = default;

        virtual boost::uuids::uuid createAsset(AssetType assetType) = 0;
        virtual boost::uuids::uuid createAsset(AssetType assetType,std::string lookUpName) = 0;

        virtual std::optional<boost::uuids::uuid> registerAsset(std::string path) = 0;
        virtual std::optional<boost::uuids::uuid> registerAsset(std::string path,std::string lookUpName) = 0;

        virtual std::optional<boost::uuids::uuid> getAssetUuid(std::string lookupName) = 0;

        virtual std::any getAssetData(const boost::uuids::uuid& id) = 0;

        template<typename T>
        T* getAssetData(const boost::uuids::uuid& id) {
            return std::any_cast<T*>(getAssetData(id));
        }

        virtual std::any getAssetData(std::string lookupName) = 0;

        template<typename T>
        T* getAssetData(std::string lookupName) {
            return std::any_cast<T*>(getAssetData(lookupName));
        }

        virtual std::optional<std::shared_ptr<AssetInfo>> getAssetInfo(const boost::uuids::uuid& id) const = 0;

        virtual std::vector<std::string> getRegisteredAssetsNames() const = 0;
        virtual std::vector<std::string> getRegisteredAssetsNames(AssetType type) const = 0;

        virtual std::vector<boost::uuids::uuid> getRegisteredAssetsUuids() const = 0;
        virtual std::vector<boost::uuids::uuid> getRegisteredAssetsUuids(AssetType type) const = 0;

    };
}

#endif //ASSETMANAGERINTERFACE_H
