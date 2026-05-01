#include "TextureAsset.h"
#include "../../AssetManager.hpp"
#include "../../JsonHelpers.hpp"
#include "stb_image.h"

namespace am
{
    TextureAsset::TextureAsset(const boost::uuids::uuid& id) : Asset(id)
    {
    }

    TextureAsset::TextureAsset(const boost::uuids::uuid& id, ImportContext assetFactoryData)
        : Asset(id, assetFactoryData)
    {
        loadFromFile(assetFactoryData.importPath);
    }

    TextureAsset::TextureAsset(const boost::uuids::uuid& id, const std::string& path, AssetFormat format) : Asset(id, path, format)
    {
        if (format == AssetFormat::Json) {
            rapidjson::Document document;
            if (!loadJsonFromFile(path, document)) {
                spdlog::error("Failed to load TextureAsset from JSON: {}", path);
                return;
            }

            if (document.HasMember("uuid") && document["uuid"].IsString()) {
                std::string savedUuidStr = document["uuid"].GetString();
                boost::uuids::uuid savedUuid = boost::uuids::string_generator()(savedUuidStr);
                if (savedUuid != id) {
                    spdlog::warn("Texture asset UUID mismatch in {}: expected {}, got {}", path.c_str(), boost::uuids::to_string(id).c_str(), boost::uuids::to_string(savedUuid).c_str());
                }
            }
            
            if (document.HasMember("type") && document["type"].IsString()) {
                std::string typeStr = document["type"].GetString();
                if (typeStr == "Texture2D") data.type = TextureType::Texture2D;
                else if (typeStr == "TextureCube") data.type = TextureType::TextureCube;
            }
        } else if (format == AssetFormat::Binary) {
            std::ifstream ifs(path, std::ios::binary | std::ios::in);
            if (!ifs.is_open()) {
                spdlog::error("Failed to open file for reading binary asset: {}", path);
                return;
            }

            // Read magic number
            char magic[6];
            ifs.read(magic, sizeof(magic));
            if (std::string(magic) != "RTEX_") {
                spdlog::error("Invalid magic number in binary texture asset: {}", path);
                return;
            }

            // Read UUID
            boost::uuids::uuid savedId;
            ifs.read(reinterpret_cast<char*>(&savedId), 16);
            if (savedId != id) {
                spdlog::warn("Texture asset UUID mismatch in {}: expected {}, got {}", path.c_str(), boost::uuids::to_string(id).c_str(), boost::uuids::to_string(savedId).c_str());
            }

            // Read metadata
            ifs.read(reinterpret_cast<char*>(&data.width), sizeof(data.width));
            ifs.read(reinterpret_cast<char*>(&data.height), sizeof(data.height));
            ifs.read(reinterpret_cast<char*>(&data.channels), sizeof(data.channels));
            ifs.read(reinterpret_cast<char*>(&data.hasAlpha), sizeof(data.hasAlpha));
            ifs.read(reinterpret_cast<char*>(&data.type), sizeof(data.type));

            // Read pixels
            size_t pixelCount;
            ifs.read(reinterpret_cast<char*>(&pixelCount), sizeof(pixelCount));
            data.pixels.resize(pixelCount);
            if (pixelCount > 0) {
                ifs.read(reinterpret_cast<char*>(data.pixels.data()), pixelCount * sizeof(std::uint32_t));
            }

            ifs.close();
        }
    }

    void TextureAsset::SaveAssetToJson(rapidjson::Document& document)
    {
        auto& allocator = document.GetAllocator();
        if (!document.IsObject()) {
            document.SetObject();
        }

        document.AddMember("uuid", rapidjson::Value(boost::uuids::to_string(id).c_str(), allocator), allocator);

        std::string typeStr;
        switch (data.type) {
            case TextureType::Texture2D: typeStr = "Texture2D"; break;
            case TextureType::TextureCube: typeStr = "TextureCube"; break;
        }
        document.AddMember("type", rapidjson::Value(typeStr.c_str(), allocator), allocator);
    }

    void TextureAsset::SaveAssetToBin(std::string& path)
    {
        std::ofstream ofs(path, std::ios::binary | std::ios::out);
        if (!ofs.is_open()) {
            spdlog::error("Failed to open file for writing binary asset: {}", path);
            return;
        }

        // Write magic number
        ofs.write("RTEX_", 6);

        // Write UUID
        ofs.write(reinterpret_cast<const char*>(&id), 16);

        // Write metadata
        ofs.write(reinterpret_cast<const char*>(&data.width), sizeof(data.width));
        ofs.write(reinterpret_cast<const char*>(&data.height), sizeof(data.height));
        ofs.write(reinterpret_cast<const char*>(&data.channels), sizeof(data.channels));
        ofs.write(reinterpret_cast<const char*>(&data.hasAlpha), sizeof(data.hasAlpha));
        ofs.write(reinterpret_cast<const char*>(&data.type), sizeof(data.type));

        // Write pixels
        size_t pixelCount = data.pixels.size();
        ofs.write(reinterpret_cast<const char*>(&pixelCount), sizeof(pixelCount));
        if (pixelCount > 0) {
            ofs.write(reinterpret_cast<const char*>(data.pixels.data()), pixelCount * sizeof(std::uint32_t));
        }

        ofs.close();
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
