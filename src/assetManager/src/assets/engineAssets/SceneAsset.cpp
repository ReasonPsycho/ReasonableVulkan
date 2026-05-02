#include "SceneAsset.h"
#include <boost/uuid/uuid_io.hpp>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <fstream>

namespace am {
    SceneAsset::SceneAsset(const boost::uuids::uuid& id) : Asset(id) {
        sceneData.SetObject();
    }

    SceneAsset::SceneAsset(const boost::uuids::uuid& id, const ImportContext& assetFactoryData) : Asset(id, assetFactoryData) {
        loadJsonFromFile(assetFactoryData.importPath, sceneData);
    }

    SceneAsset::SceneAsset(const std::string& path, AssetFormat format) : Asset(path, format) {
        if (format == AssetFormat::Binary) {
            std::ifstream ifs(path, std::ios::binary);
            if (!ifs.is_open()) return;

            char magic[6];
            ifs.read(magic, sizeof(SCENE_MAGIC));

            ifs.read(reinterpret_cast<char*>(&id), 16);

            size_t dataSize;
            ifs.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));

            std::string jsonStr(dataSize, '\0');
            ifs.read(&jsonStr[0], dataSize);

            sceneData.Parse(jsonStr.c_str());
        } else {
            loadJsonFromFile(path, sceneData);
            if (sceneData.HasMember("uuid") && sceneData["uuid"].IsString()) {
                id = boost::uuids::string_generator()(sceneData["uuid"].GetString());
            }
        }
    }

    AssetType SceneAsset::getType() const { return AssetType::Scene; }

    size_t SceneAsset::calculateContentHash() const {
        return std::hash<std::string>{}(boost::uuids::to_string(id));
    }

    std::any SceneAsset::getAssetData() { return &sceneData; }

    void SceneAsset::SaveAssetToJson(rapidjson::Document& document) {
        document.CopyFrom(sceneData, document.GetAllocator());
    }

    void SceneAsset::SaveAssetToBin(std::string& path) {
        std::ofstream ofs(path, std::ios::binary | std::ios::out);
        if (!ofs.is_open()) return;

        ofs.write(SCENE_MAGIC, sizeof(SCENE_MAGIC));
        ofs.write(reinterpret_cast<const char*>(&id), 16);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        sceneData.Accept(writer);

        size_t dataSize = buffer.GetSize();
        ofs.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
        ofs.write(buffer.GetString(), dataSize);

        ofs.close();
    }


    void SceneAsset::SaveAssetMetadata(rapidjson::Document& document) {}
    void SceneAsset::LoadAssetMetadata(rapidjson::Document& document) {}
}
