//
// Created by redkc on 07/08/2025.
//

#ifndef ASSETMANAGERINTERFACE_H
#define ASSETMANAGERINTERFACE_H
#include <boost/uuid/uuid.hpp>
#include "AssetInfo.hpp"


namespace am
{
    class AssetManagerInterface{
    public:
        virtual ~AssetManagerInterface() = default;
        virtual std::optional<std::shared_ptr<am::AssetInfo>> registerAsset(std::string path) = 0;
        virtual  std::optional<std::shared_ptr<am::AssetInfo>> getAssetInfo(const boost::uuids::uuid& id) const = 0;
        virtual std::optional<am::Asset*> getAsset(const boost::uuids::uuid &id) const = 0;
    };
}

#endif //ASSETMANAGERINTERFACE_H
