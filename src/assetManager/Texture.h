#ifndef TEXTURE_H
#define TEXTURE_H

#include "AssetFactoryRegistry.hpp"
#include <string>
#include <vector>
#include <cstdint>
#include "Asset.hpp"
#include "spdlog/spdlog.h"
#include <functional>


namespace ae {
    struct TextureData {
        std::vector<std::uint8_t> pixels;
        int width{0};
        int height{0};
        int channels{0};
        bool hasAlpha{false};
    };

    class Texture : public Asset {
        public:
        static inline AssetFactoryRegistry::Registrar<Texture,BaseFactoryContext> registrar{AssetType::Texture};
        explicit Texture(ae::BaseFactoryContext base_factory_context);

        ~Texture() override;

        void loadFromFile(const std::string &path);

        [[nodiscard]] size_t calculateContentHash() const override;
        [[nodiscard]] AssetType getType() const override;

        [[nodiscard]] int getWidth() const { return textureData ? textureData->width : 0; }
        [[nodiscard]] int getHeight() const { return textureData ? textureData->height : 0; }
        [[nodiscard]] int getChannels() const { return textureData ? textureData->channels : 0; }
        [[nodiscard]] bool hasAlpha() const { return textureData ? textureData->hasAlpha : false; }

        [[nodiscard]] const uint8_t *getData() const {
            return textureData ? textureData->pixels.data() : nullptr;
        }

        [[nodiscard]] size_t getDataSize() const {
            return textureData ? textureData->pixels.size() : 0;
        }

    private:
        std::unique_ptr<TextureData> textureData;
    };
}

#endif