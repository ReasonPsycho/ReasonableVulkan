//
// Created by redkc on 07/08/2025.
//

#ifndef TEXTUREDATA_H
#define TEXTUREDATA_H
#include <cstdint>
#include <vector>


namespace am
{
    struct TextureData {
        std::vector<std::uint8_t> pixels;
        int width{0};
        int height{0};
        int channels{0};
        bool hasAlpha{false};
    };
}
#endif //TEXTUREDATA_H
