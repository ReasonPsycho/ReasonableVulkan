//
// Created by redkc on 02/12/2023.
//

#ifndef OPENGLGP_MODEL_H
#define OPENGLGP_MODEL_H

using namespace std;

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "../../include/assetDatas/Node.h"
#include "../../include/Asset.hpp"
#include "assetDatas/ModelData.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "map"
#include <string>
#include <vector>
#include "meshAsset/MeshAsset.h"

namespace am {

    class ModelAsset : public Asset {
    public:
        // model data
        ModelData data;

        explicit ModelAsset(const boost::uuids::uuid& id);
        ModelAsset(const boost::uuids::uuid& id, ImportContext assetFactoryData);
        explicit ModelAsset(const boost::uuids::uuid& id, const std::string& path, AssetFormat format);

        void SaveAssetToJson(rapidjson::Document& document) override;
        void SaveAssetToBin(std::string& path) override {}

        void loadFromFile(ImportContext base_factory_context);

        static int getMeshIndexInScene(const aiScene *scene, const aiMesh *targetMesh);

        [[nodiscard]] AssetType getType() const override;

        [[nodiscard]] size_t calculateContentHash() const override;
        void SaveAssetMetadata(rapidjson::Document& document) override {}
        void LoadAssetMetadata(rapidjson::Document& document) override {}

        any getAssetData() override {
            return &data;
        }
    private:

        Node processNode(ImportContext baseFactoryContext, aiNode *aiNode, const aiScene *scene);
        boost::uuids::uuid processMesh(ImportContext baseFactoryContext, aiMesh* mesh, const aiScene* scene);
    };
}

#endif //OPENGLGP_MODEL_H
