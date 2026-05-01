#ifndef REASONABLEVULKAN_SCENEASSET_H
#define REASONABLEVULKAN_SCENEASSET_H

#include "../../../include/Asset.hpp"
#include "../../JsonHelpers.hpp"

namespace am {
    const char SCENE_MAGIC[] = "RSCNE";

    class SceneAsset : public Asset {
    public:
        rapidjson::Document sceneData;

        explicit SceneAsset(const boost::uuids::uuid& id);

        SceneAsset(const boost::uuids::uuid& id, ImportContext assetFactoryData);

        explicit SceneAsset(const boost::uuids::uuid& id, const std::string& path, AssetFormat format);

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
