#include "ConfigAsset.h"
#include <boost/uuid/uuid_io.hpp>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <fstream>
#include <boost/uuid/string_generator.hpp>

namespace am {
    ConfigAsset::ConfigAsset(const boost::uuids::uuid& id) : Asset(id) {
        configData.SetObject();
    }

    ConfigAsset::ConfigAsset(const boost::uuids::uuid& id, const ImportContext& assetFactoryData) : Asset(id, assetFactoryData) {
        loadJsonFromFile(assetFactoryData.importPath, configData);
    }

    ConfigAsset::ConfigAsset(const std::string& path, AssetFormat format) : Asset(path, format) {
        if (format == AssetFormat::Binary) {
            std::ifstream ifs(path, std::ios::binary);
            if (!ifs.is_open()) return;

            char magic[6];
            ifs.read(magic, sizeof(CONFIG_MAGIC));

            ifs.read(reinterpret_cast<char*>(&id), 16);

            size_t dataSize;
            ifs.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));

            std::string jsonStr(dataSize, '\0');
            ifs.read(&jsonStr[0], dataSize);

            configData.Parse(jsonStr.c_str());
        } else {
            loadJsonFromFile(path, configData);
            if (configData.HasMember("uuid") && configData["uuid"].IsString()) {
                id = boost::uuids::string_generator()(configData["uuid"].GetString());
            }
        }
    }

    AssetType ConfigAsset::getType() const { return AssetType::Config; }

    size_t ConfigAsset::calculateContentHash() const {
        return std::hash<std::string>{}(boost::uuids::to_string(id));
    }

    std::any ConfigAsset::getAssetData() { return &configData; }

    void ConfigAsset::SaveAssetToJson(rapidjson::Document& document) {
        document.CopyFrom(configData, document.GetAllocator());
    }

    void ConfigAsset::SaveAssetToBin(std::string& path) {
        std::ofstream ofs(path, std::ios::binary | std::ios::out);
        if (!ofs.is_open()) return;

        ofs.write(CONFIG_MAGIC, sizeof(CONFIG_MAGIC));
        ofs.write(reinterpret_cast<const char*>(&id), 16);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        configData.Accept(writer);

        size_t dataSize = buffer.GetSize();
        ofs.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
        ofs.write(buffer.GetString(), dataSize);

        ofs.close();
    }


    void ConfigAsset::SaveAssetMetadata(rapidjson::Document& document) {}
    void ConfigAsset::LoadAssetMetadata(rapidjson::Document& document) {}
}
