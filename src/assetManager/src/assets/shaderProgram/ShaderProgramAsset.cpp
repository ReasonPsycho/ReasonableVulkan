#include "ShaderProgramAsset.h"
#include <fstream>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <spdlog/spdlog.h>
#include <filesystem>

namespace am {

    ShaderProgramAsset::ShaderProgramAsset(AssetFactoryData &assetFactoryData) : Asset(assetFactoryData) {
    }

    void ShaderProgramAsset::LoadAssetFromImport(AssetFactoryData assetFactoryData) {
        loadFromJson(assetFactoryData.path);
    }

    void ShaderProgramAsset::loadFromJson(const std::string& path) {
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

        AssetManager &assetManager = AssetManager::getInstance();
        std::filesystem::path basePath = std::filesystem::path(path).parent_path();

        auto loadStage = [&](const char* key, std::shared_ptr<AssetInfo>& target) {
            if (doc.HasMember(key) && doc[key].IsString()) {
                std::string stagePath = doc[key].GetString();
                std::filesystem::path fullPath = basePath / stagePath;
                auto result = assetManager.registerAsset(fullPath.string());
                if (result) {
                    target = result.value();
                } else {
                    spdlog::warn("Failed to register shader stage: {} for program: {}", stagePath, path);
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
