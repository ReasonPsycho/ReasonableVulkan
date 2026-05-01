#include "ShaderProgramAsset.h"
#include "../../AssetManager.hpp"
#include "../../JsonHelpers.hpp"
#include <fstream>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <spdlog/spdlog.h>
#include <filesystem>

namespace am {
    ShaderProgramAsset::ShaderProgramAsset(const boost::uuids::uuid& id) : Asset(id) {
    }

    ShaderProgramAsset::ShaderProgramAsset(const boost::uuids::uuid& id, ImportContext assetFactoryData) : Asset(id, assetFactoryData) {
        importFromImportJson(assetFactoryData.importPath);
    }

    ShaderProgramAsset::ShaderProgramAsset(const boost::uuids::uuid& id, const std::string& path, AssetFormat format) : Asset(id, path, format) {
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

        auto addStage = [&](const char* key, std::shared_ptr<AssetInfo>& asset) {
            if (asset) {
                std::string uuidStr = boost::uuids::to_string(asset->id);
                document.AddMember(rapidjson::StringRef(key), rapidjson::Value(uuidStr.c_str(), allocator), allocator);
            }
        };

        addStage("vertex", data.vertexShader);
        addStage("fragment", data.fragmentShader);
        addStage("compute", data.computeShader);
        addStage("geometry", data.geometryShader);
        addStage("tessellationControl", data.tessellationControlShader);
        addStage("tessellationEvaluation", data.tessellationEvaluationShader);
    }


    void ShaderProgramAsset::importFromImportJson(const std::string& path) {
        std::ifstream ifs(path);
        if (!ifs.is_open()) {
            spdlog::error("Failed to open shader import file: {}", path);
            return;
        }

        rapidjson::IStreamWrapper isw(ifs);
        rapidjson::Document doc;
        doc.ParseStream(isw);

        if (doc.HasParseError()) {
            spdlog::error("Failed to parse shader import JSON: {}", path);
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
        std::ifstream ifs(path);
        if (!ifs.is_open()) {
            spdlog::error("Failed to open shader program file: {}", path);
            return;
        }

        rapidjson::IStreamWrapper isw(ifs);
        rapidjson::Document doc;
        doc.ParseStream(isw);

        if (doc.HasParseError()) {
            spdlog::error("Failed to parse shader program JSON: {}", path);
            return;
        }

        if (doc.HasMember("uuid") && doc["uuid"].IsString()) {
            std::string savedUuidStr = doc["uuid"].GetString();
            boost::uuids::uuid savedUuid = boost::uuids::string_generator()(savedUuidStr);
            if (savedUuid != id) {
                spdlog::warn("Shader program asset UUID mismatch in {}: expected {}, got {}", path, boost::uuids::to_string(id), savedUuidStr);
            }
        }

        AssetManager &assetManager = AssetManager::getInstance();

        auto loadStage = [&](const char* key, std::shared_ptr<AssetInfo>& target) {
            if (doc.HasMember(key) && doc[key].IsString()) {
                std::string value = doc[key].GetString();
                try {
                    boost::uuids::uuid shaderId = boost::uuids::string_generator()(value);
                    target = assetManager.getAssetInfo(shaderId).value_or(nullptr);
                    if (!target) {
                        spdlog::warn("Shader asset with UUID {} not found for program {}", value, path);
                    }
                } catch (const std::exception& e) {
                    spdlog::error("Failed to parse shader UUID {} for program {}: {}", value, path, std::string(e.what()));
                }
            }
        };

        loadStage("vertex", data.vertexShader);
        loadStage("fragment", data.fragmentShader);
        loadStage("compute", data.computeShader);
        loadStage("geometry", data.geometryShader);
        loadStage("tessellationControl", data.tessellationControlShader);
        loadStage("tessellationEvaluation", data.tessellationEvaluationShader);
    }

    size_t ShaderProgramAsset::calculateContentHash() const {
        size_t hash = 0;
        auto combineHash = [&](const std::shared_ptr<AssetInfo>& asset) {
            if (asset) {
                hash ^= asset->contentHash + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            }
        };

        combineHash(data.vertexShader);
        combineHash(data.fragmentShader);
        combineHash(data.computeShader);
        combineHash(data.geometryShader);
        combineHash(data.tessellationControlShader);
        combineHash(data.tessellationEvaluationShader);

        return hash;
    }

    AssetType ShaderProgramAsset::getType() const {
        return AssetType::ShaderProgram;
    }

} // namespace am
