//
// Created by redkc on 02/12/2023.
//

#ifndef OPENGLGP_MESH_H
#define OPENGLGP_MESH_H
#include <tiny_gltf.h>

#include "../../../include/Asset.hpp"
#include <assimp/scene.h>
#include "../../AssetFactoryRegistry.hpp"
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

#include "../material/Material.hpp"
#include "assetDatas/MeshData.h"

namespace am {
    struct Vertex;
}

using namespace std;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <vector>

#include "../shader/Shader.h"
#include "../texture/Texture.h"

// Specialized context for textures


namespace am {
    class Mesh : public Asset {
    public:
        static inline AssetFactoryRegistry::Registrar<Mesh> registrar{AssetType::Mesh};

        explicit Mesh(AssetFactoryData meshFactoryContext); //This maby someday should intake a interface of materials

        //This maby someday should intake a interface of materials
        size_t calculateContentHash() const override;
        [[nodiscard]] AssetType getType() const override;

        void* getAssetData() override { return &data; }

    private:
        MeshData data;
    };
}


#endif //OPENGLGP_MESH_H
