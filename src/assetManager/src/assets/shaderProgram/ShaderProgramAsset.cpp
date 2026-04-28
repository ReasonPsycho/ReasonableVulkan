#include "ShaderProgramAsset.h"
#include "../../AssetManager.hpp"
#include "../../JsonHelpers.hpp"
#include <fstream>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <spdlog/spdlog.h>
#include <filesystem>

namespace am {

    ShaderProgramAsset::ShaderProgramAsset(const boost::uuids::uuid& id, ImportContext assetFactoryData) : Asset(id, assetFactoryData) {
        loadFromJson(assetFactoryData.importPath);
    }

    ShaderProgramAsset::ShaderProgramAsset(const boost::uuids::uuid& id, const std::string& path, AssetFormat format) : Asset(id, path, format) {
        if (format == AssetFormat::Json) {
            loadFromJson(path);
        }
    }

    void ShaderProgramAsset::SaveAssetToJson(rapidjson::Document& document) {
        auto& allocator = document.GetAllocator();
        if (!document.IsObject()) {
            document.SetObject();
        }

        auto addStage = [&](const char* key, std::shared_ptr<AssetInfo>& asset) {
            if (asset) {
                document.AddMember(rapidjson::StringRef(key), rapidjson::Value(asset->importPath.c_str(), allocator), allocator);
            }
        };

        addStage("vertex", data.vertexShader);
        addStage("fragment", data.fragmentShader);
        addStage("compute", data.computeShader);
        addStage("geometry", data.geometryShader);
        addStage("tessellationControl", data.tessellationControlShader);
        addStage("tessellationEvaluation", data.tessellationEvaluationShader);
    }


    void ShaderProgramAsset::loadFromJson(const std::string& path) {
        std::ifstream ifs(path);
        if (!ifs.is_open()) {
            spdlog::error("Failed to open shader program file");
            return;
        }

        rapidjson::IStreamWrapper isw(ifs);
        rapidjson::Document doc;
        doc.ParseStream(isw);

        if (doc.HasParseError()) {
            spdlog::error("Failed to parse shader program JSON");
            return;
        }

        AssetManager &assetManager = AssetManager::getInstance();
        std::filesystem::path basePath = std::filesystem::path(path).parent_path();

        auto loadStage = [&](const char* key, std::shared_ptr<AssetInfo>& target) {
            if (doc.HasMember(key) && doc[key].IsString()) {
                std::string stagePath = doc[key].GetString();
                std::filesystem::path fullPath = (basePath / stagePath).lexically_normal();
                auto result = assetManager.registerAsset(fullPath.string());
                if (result) {
                    target = assetManager.getAssetInfo(result.value()).value_or(nullptr);
                } else {
                spdlog::warn("Failed to register shader stage for program");
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
