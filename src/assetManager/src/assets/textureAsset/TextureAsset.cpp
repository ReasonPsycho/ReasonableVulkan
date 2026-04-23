#include "TextureAsset.h"
#include "../../AssetManager.hpp"
#include "../../JsonHelpers.hpp"
#include "stb_image.h"

namespace am
{
    TextureAsset::TextureAsset(ImportContext assetFactoryData)
        : Asset(assetFactoryData)
    {
        loadFromFile(assetFactoryData.importPath);
    }

    TextureAsset::TextureAsset(const std::string& path, AssetFormat format) : Asset(path, format)
    {
        if (format == AssetFormat::Json) {
            rapidjson::Document document;
            if (!loadJsonFromFile(path, document)) {
                spdlog::error("Failed to load TextureAsset from JSON: {}", path);
                return;
            }
            if (document.HasMember("width") && document["width"].IsUint()) data.width = document["width"].GetUint();
            if (document.HasMember("height") && document["height"].IsUint()) data.height = document["height"].GetUint();
            if (document.HasMember("channels") && document["channels"].IsUint()) data.channels = document["channels"].GetUint();
            if (document.HasMember("hasAlpha") && document["hasAlpha"].IsBool()) data.hasAlpha = document["hasAlpha"].GetBool();

            if (document.HasMember("type") && document["type"].IsString()) {
                std::string typeStr = document["type"].GetString();
                if (typeStr == "Texture2D") data.type = TextureType::Texture2D;
                else if (typeStr == "TextureCube") data.type = TextureType::TextureCube;
            }
        }
    }

    void TextureAsset::SaveAssetToJson(rapidjson::Document& document)
    {
        auto& allocator = document.GetAllocator();
        if (!document.IsObject()) {
            document.SetObject();
        }

        document.AddMember("width", data.width, allocator);
        document.AddMember("height", data.height, allocator);
        document.AddMember("channels", data.channels, allocator);
        document.AddMember("hasAlpha", data.hasAlpha, allocator);

        std::string typeStr;
        switch (data.type) {
            case TextureType::Texture2D: typeStr = "Texture2D"; break;
            case TextureType::TextureCube: typeStr = "TextureCube"; break;
        }
        document.AddMember("type", rapidjson::Value(typeStr.c_str(), allocator), allocator);
    }


    TextureAsset::~TextureAsset() = default;

    void TextureAsset::loadFromFile(const std::string& path)
    {
        // Force stb_image to return 4 channels for consistency
        int width, height, channels;
        stbi_set_flip_vertically_on_load(true); // Flip images vertically

        uint8_t* fileData = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

        if (!fileData)
        {
            spdlog::error("Failed to load texture");
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

        data.type = TextureType::Texture2D;

        spdlog::info("Loaded texture");
    }

    size_t TextureAsset::calculateContentHash() const
    {
        size_t hash = 0;

        // Hash basic properties
        hash ^= std::hash<int>{}(data.width);
        hash ^= std::hash<int>{}(data.height) << 1;
        hash ^= std::hash<int>{}(data.channels) << 2;

        // Hash pixel data in chunks to improve performance
        const size_t chunkSize = 1024; // Process 1KB at a time
        const uint32_t* pixels = data.pixels.data();
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

    AssetType TextureAsset::getType() const
    {
        return AssetType::Texture;
    }

    void TextureAsset::SaveAssetMetadata(rapidjson::Document& document)
    {
        auto& allocator = document.GetAllocator();

        std::string typeStr;
        switch (data.type) {
        case TextureType::Texture2D: typeStr = "Texture2D"; break;
        case TextureType::TextureCube: typeStr = "TextureCube"; break;
        }

        document.AddMember(
            rapidjson::Value("type", allocator),
            rapidjson::Value(typeStr.c_str(), allocator),
            allocator
        );
    }

    void TextureAsset::LoadAssetMetadata(rapidjson::Document& document)
    {
        if (document.HasMember("type") && document["type"].IsString()) {
            std::string typeStr = document["type"].GetString();
            if (typeStr == "Texture2D") {
                data.type = TextureType::Texture2D;
            } else if (typeStr == "TextureCube") {
                data.type = TextureType::TextureCube;
            }
        }
    }
}
