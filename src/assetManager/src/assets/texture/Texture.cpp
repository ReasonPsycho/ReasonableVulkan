#include "Texture.h"

namespace am
{
    Texture::Texture(am::AssetFactoryData base_factory_context)
        : Asset(base_factory_context)
    {
        loadFromFile(base_factory_context.path);
    }

    Texture::~Texture() = default;

    void Texture::loadFromFile(const std::string& path)
    {
        // Force stb_image to return 4 channels for consistency
        int width, height, channels;
        stbi_set_flip_vertically_on_load(true); // Flip images vertically

        uint8_t* fileData = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

        if (!fileData)
        {
            spdlog::error("Failed to load texture: {} - {}", path, stbi_failure_reason());
            throw std::runtime_error("Failed to load texture: " + path);
        }

        // Create new texture data
        data.width = width;
        data.height = height;
        data.channels = 4; // We forced RGBA
        data.hasAlpha = true;

        // Calculate total size and copy data
        size_t dataSize = width * height * 4; // 4 for RGBA
        data.pixels.resize(dataSize);
        std::memcpy(data.pixels.data(), fileData, dataSize);

        // Free the stb_image data
        stbi_image_free(fileData);

        spdlog::info("Loaded texture: {} ({}x{}, {} channels)",
                     path, width, height, channels);
    }

    size_t Texture::calculateContentHash() const
    {
        size_t hash = 0;

        // Hash basic properties
        hash ^= std::hash<int>{}(data.width);
        hash ^= std::hash<int>{}(data.height) << 1;
        hash ^= std::hash<int>{}(data.channels) << 2;

        // Hash pixel data in chunks to improve performance
        const size_t chunkSize = 1024; // Process 1KB at a time
        const uint8_t* pixels = data.pixels.data();
        const size_t totalSize = data.pixels.size();

        for (size_t i = 0; i < totalSize; i += chunkSize)
        {
            size_t remaining = std::min(chunkSize, totalSize - i);
            for (size_t j = 0; j < remaining; ++j)
            {
                hash ^= std::hash<uint8_t>{}(pixels[i + j]) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            }
        }

        return hash;
    }

    AssetType Texture::getType() const
    {
        return AssetType::Texture;
    }
}
