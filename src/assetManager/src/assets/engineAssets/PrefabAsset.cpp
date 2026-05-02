#include "PrefabAsset.h"
#include <boost/uuid/uuid_io.hpp>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <fstream>

namespace am {
    PrefabAsset::PrefabAsset(const boost::uuids::uuid& id) : Asset(id) {
        prefabData.SetObject();
    }

    PrefabAsset::PrefabAsset(const boost::uuids::uuid& id, const ImportContext& assetFactoryData) : Asset(id, assetFactoryData) {
        loadJsonFromFile(assetFactoryData.importPath, prefabData);
    }

    PrefabAsset::PrefabAsset(const std::string& path, AssetFormat format) : Asset(path, format) {
        if (format == AssetFormat::Binary) {
            std::ifstream ifs(path, std::ios::binary);
            if (!ifs.is_open()) return;

            char magic[6];
            ifs.read(magic, sizeof(PREFAB_MAGIC));

            ifs.read(reinterpret_cast<char*>(&id), 16);

            size_t dataSize;
            ifs.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));

            std::string jsonStr(dataSize, '\0');
            ifs.read(&jsonStr[0], dataSize);

            prefabData.Parse(jsonStr.c_str());
        } else {
            loadJsonFromFile(path, prefabData);
            if (prefabData.HasMember("uuid") && prefabData["uuid"].IsString()) {
                id = boost::uuids::string_generator()(prefabData["uuid"].GetString());
            }
        }
    }

    AssetType PrefabAsset::getType() const { return AssetType::Prefab; }

    size_t PrefabAsset::calculateContentHash() const {
        return std::hash<std::string>{}(boost::uuids::to_string(id));
    }

    std::any PrefabAsset::getAssetData() { return &prefabData; }

    void PrefabAsset::SaveAssetToJson(rapidjson::Document& document) {
        document.CopyFrom(prefabData, document.GetAllocator());
    }

    void PrefabAsset::SaveAssetToBin(std::string& path) {
        std::ofstream ofs(path, std::ios::binary | std::ios::out);
        if (!ofs.is_open()) return;

        ofs.write(PREFAB_MAGIC, sizeof(PREFAB_MAGIC));
        ofs.write(reinterpret_cast<const char*>(&id), 16);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        prefabData.Accept(writer);

        size_t dataSize = buffer.GetSize();
        ofs.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
        ofs.write(buffer.GetString(), dataSize);

        ofs.close();
    }


    void PrefabAsset::SaveAssetMetadata(rapidjson::Document& document) {}
    void PrefabAsset::LoadAssetMetadata(rapidjson::Document& document) {}
}
