#ifndef TEXTURE_H
#define TEXTURE_H

#include "../../AssetFactoryRegistry.hpp"
#include <string>
#include <vector>
#include <cstdint>
#include "../../../include/Asset.hpp"
#include "spdlog/spdlog.h"
#include <functional>

#include "assetDatas/TextureData.h"


namespace am {
    
    class TextureAsset : public Asset {
    public:
        static inline AssetFactoryRegistry::Registrar<TextureAsset> registrar{AssetType::Texture};

        explicit TextureAsset(am::AssetFactoryData base_factory_context);

        ~TextureAsset() override;

        void loadFromFile(const std::string &path);

        [[nodiscard]] size_t calculateContentHash() const override;
        [[nodiscard]] AssetType getType() const override;

        [[nodiscard]] int getWidth() const { return data.width ; }
        [[nodiscard]] int getHeight() const { return data.height ; }
        [[nodiscard]] int getChannels() const { return data.channels ; }
        [[nodiscard]] bool hasAlpha() const { return data.hasAlpha; }

        [[nodiscard]] const uint8_t *getData() const {
            return data.pixels.data();
        }

        [[nodiscard]] size_t getDataSize() const {
            return data.pixels.size() ;
        }

        void* getAssetData() override { return &data; }
    private:
        TextureData data;
    };
}

#endif