//
// Created by redkc on 02/12/2023.
//

#ifndef OPENGLGP_MESH_H
#define OPENGLGP_MESH_H
#include "Asset.hpp"
#include <assimp/scene.h>
#include "AssetFactoryRegistry.hpp"

namespace ae {
    struct Vertex;
}

using namespace std;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <vector>

#include "Shader.h"
#include "Texture.h"

// Specialized context for textures


namespace ae {
#define MAX_BONE_INFLUENCE 4

struct Vertex {
    // position
    glm::vec3 Position;
    // normal
    glm::vec3 Normal;
    // texCoords
    glm::vec2 TexCoords;
    // tangent
    glm::vec3 Tangent;
    // bitangent
    glm::vec3 Bitangent;
    //bone indexes which will influence this vertex
    int m_BoneIDs[MAX_BONE_INFLUENCE];
    //weights from each bone
    float m_Weights[MAX_BONE_INFLUENCE];
};
    
    class Mesh : public Asset {
    public:
        static inline AssetFactoryRegistry::Registrar<Mesh> registrar{AssetType::Mesh};

        // mesh Data
        vector<Vertex> vertices;
        vector<unsigned int> indices;

        explicit Mesh(AssetFactoryData meshFactoryContext); //This maby someday should intake a interface of materials
        void ExtractMeshData(AssetFactoryData meshFactoryContext, const aiScene *scene);

        explicit Mesh(AssetFactoryData meshFactoryContext, const aiScene *scene ); //This maby someday should intake a interface of materials
        size_t calculateContentHash() const override;

        [[nodiscard]] AssetType getType() const override;
    };
}


#endif //OPENGLGP_MESH_H
