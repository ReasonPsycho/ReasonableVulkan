//
// Created by redkc on 07/08/2025.
//

#ifndef TEXTUREDATA_H
#define TEXTUREDATA_H
#include <cstdint>
#include <vector>


namespace am
{
    enum class TextureType {
        Texture2D,
        TextureCube
    };

    struct TextureData {
        std::vector<std::uint32_t> pixels;
        uint32_t width{0};
        uint32_t height{0};
        uint32_t channels{0};
        bool hasAlpha{false};
        TextureType type;
        std::string originalSource;
    };
}
#endif //TEXTUREDATA_H
