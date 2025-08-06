//
// Created by redkc on 02/12/2023.
//

#ifndef OPENGLGP_MODEL_H
#define OPENGLGP_MODEL_H


#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Node.h"
#include "../Asset.hpp"
#include "../AssetFactoryRegistry.hpp"

using namespace std;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "map"
#include <string>
#include <vector>
#include "../AssimpGLMHelpers.h"
#include "texture/Texture.h"
#include "mesh/Mesh.h"

namespace am {

    class Model : public Asset {
    public:
        static inline AssetFactoryRegistry::Registrar<Model> registrar{AssetType::Model};
        // model data 
        Node rootNode;

        explicit Model(AssetFactoryData base_factory_context): Asset(base_factory_context) {
            loadFromFile(base_factory_context);
        };

        void loadFromFile(AssetFactoryData base_factory_context);

        static int getMeshIndexInScene(const aiScene *scene, const aiMesh *targetMesh);

        [[nodiscard]] AssetType getType() const override;

        [[nodiscard]] size_t calculateContentHash() const override;

    private:

        Node processNode(AssetFactoryData baseFactoryContext, aiNode *aiNode, const aiScene *scene);

        std::shared_ptr<AssetInfo> processMesh(AssetFactoryData baseFactoryContext, aiMesh *mesh, const aiScene *scene);
    };
}

#endif //OPENGLGP_MODEL_H
