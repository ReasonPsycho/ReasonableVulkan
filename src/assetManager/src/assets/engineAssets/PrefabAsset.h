#ifndef REASONABLEVULKAN_PREFABASSET_H
#define REASONABLEVULKAN_PREFABASSET_H

#include "../../../include/Asset.hpp"
#include "../../JsonHelpers.hpp"

namespace am {
    const char PREFAB_MAGIC[] = "RPRFB";

    class PrefabAsset : public Asset {
    public:
        rapidjson::Document prefabData;

        explicit PrefabAsset(const boost::uuids::uuid& id);

        PrefabAsset(const boost::uuids::uuid& id, ImportContext assetFactoryData);

        explicit PrefabAsset(const boost::uuids::uuid& id, const std::string& path, AssetFormat format);

        [[nodiscard]] AssetType getType() const override;

        [[nodiscard]] size_t calculateContentHash() const override;

        std::any getAssetData() override;

        void SaveAssetToJson(rapidjson::Document& document) override;

        void SaveAssetToBin(std::string& path) override;

        void SaveAssetMetadata(rapidjson::Document& document) override;
        void LoadAssetMetadata(rapidjson::Document& document) override;
    };
}

#endif
