#ifndef REASONABLEVULKAN_CONFIGASSET_H
#define REASONABLEVULKAN_CONFIGASSET_H

#include "../../../include/Asset.hpp"
#include "../../JsonHelpers.hpp"

namespace am {
    const char CONFIG_MAGIC[] = "RCNFG";

    class ConfigAsset : public Asset {
    public:
        rapidjson::Document configData;

        explicit ConfigAsset(const boost::uuids::uuid& id);

        ConfigAsset(const boost::uuids::uuid& id, const ImportContext& assetFactoryData);

        explicit ConfigAsset(const std::string& path, AssetFormat format);

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
