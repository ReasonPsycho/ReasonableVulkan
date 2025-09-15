//
// Created by redkc on 02/12/2023.
//

#ifndef OPENGLGP_MODEL_H
#define OPENGLGP_MODEL_H


#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "../../include/assetDatas/Node.h"
#include "../../include/Asset.hpp"
#include "../AssetFactoryRegistry.hpp"
#include "assetDatas/ModelData.h"

using namespace std;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "map"
#include <string>
#include <vector>
#include "meshAsset/MeshAsset.h"

namespace am {

    class ModelAsset : public Asset {
    public:
        static inline AssetFactoryRegistry::Registrar<ModelAsset> registrar{AssetType::Model};
        // model data 
        ModelData data;

        explicit ModelAsset(AssetFactoryData base_factory_context): Asset(base_factory_context) {
            loadFromFile(base_factory_context);
        };

        void loadFromFile(AssetFactoryData base_factory_context);

        static int getMeshIndexInScene(const aiScene *scene, const aiMesh *targetMesh);

        [[nodiscard]] AssetType getType() const override;

        [[nodiscard]] size_t calculateContentHash() const override;

        void* getAssetData() override { return &data; }
    private:

        Node processNode(AssetFactoryData baseFactoryContext, aiNode *aiNode, const aiScene *scene);

        std::shared_ptr<AssetInfo> processMesh(AssetFactoryData baseFactoryContext, aiMesh *mesh, const aiScene *scene);
    };
}

#endif //OPENGLGP_MODEL_H
