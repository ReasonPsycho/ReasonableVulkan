#include "ShaderProgramAsset.h"
#include "../../AssetManager.hpp"
#include "../../JsonHelpers.hpp"
#include <fstream>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <spdlog/spdlog.h>
#include <filesystem>

namespace am {
    ShaderProgramAsset::ShaderProgramAsset(const boost::uuids::uuid& id, std::string path) : Asset(id, path) {
    }

    ShaderProgramAsset::ShaderProgramAsset(const boost::uuids::uuid& id, ImportContext assetFactoryData) : Asset(id, assetFactoryData) {
        importFromImportJson(assetFactoryData.importPath);
    }

    ShaderProgramAsset::ShaderProgramAsset(const std::string& path, AssetFormat format) : Asset(path, format) {
        if (format == AssetFormat::Json) {
            loadFromProgramJson(path);
        }
    }

    void ShaderProgramAsset::SaveAssetToJson(rapidjson::Document& document) {
        auto& allocator = document.GetAllocator();
        if (!document.IsObject()) {
            document.SetObject();
        }

        document.AddMember("uuid", rapidjson::Value(boost::uuids::to_string(id).c_str(), allocator), allocator);
        SerializeAssetData(data, document, allocator);
    }


    void ShaderProgramAsset::importFromImportJson(const std::string& path) {
        rapidjson::Document doc;
        if (!loadJsonFromFile(path, doc)) {
            return;
        }

        AssetManager &assetManager = AssetManager::getInstance();
        std::filesystem::path basePath = std::filesystem::path(path).parent_path();

        auto loadStage = [&](const char* key, std::shared_ptr<AssetInfo>& target) {
            if (doc.HasMember(key) && doc[key].IsString()) {
                std::string value = doc[key].GetString();
                std::filesystem::path shaderPath = (basePath / value).lexically_normal();
                target = assetManager.getAssetInfo(assetManager.registerAsset(shaderPath.string()).value_or(boost::uuids::nil_uuid())).value_or(nullptr);
            }
        };

        loadStage("vertex", data.vertexShader);
        loadStage("fragment", data.fragmentShader);
        loadStage("compute", data.computeShader);
        loadStage("geometry", data.geometryShader);
        loadStage("tessellationControl", data.tessellationControlShader);
        loadStage("tessellationEvaluation", data.tessellationEvaluationShader);
    }

    void ShaderProgramAsset::loadFromProgramJson(const std::string& path) {
        rapidjson::Document doc;
        if (!loadJsonFromFile(path, doc)) {
            // Error logged by loadJsonFromFile
            return;
        }

        if (doc.HasMember("uuid") && doc["uuid"].IsString()) {
            std::string savedUuidStr = doc["uuid"].GetString();
            id = boost::uuids::string_generator()(savedUuidStr);
        }

        DeserializeAssetData(data, doc, &AssetManager::getInstance());
    }

    size_t ShaderProgramAsset::calculateContentHash() const {
        return CalculateReflectedContentHash(data);
    }

    AssetType ShaderProgramAsset::getType() const {
        return AssetType::ShaderProgram;
    }

} // namespace am
