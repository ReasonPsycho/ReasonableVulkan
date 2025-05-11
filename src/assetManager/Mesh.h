//
// Created by redkc on 02/12/2023.
//

#ifndef OPENGLGP_MESH_H
#define OPENGLGP_MESH_H
#include "Asset.hpp"

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

    struct MeshFactoryContext : ae::BaseFactoryContext {
        vector<ae::Vertex> vertices;
        vector<unsigned int> indices;
    };
    
class Mesh: public Asset {
public:
    // mesh Data
    vector<Vertex> vertices;
    vector<unsigned int> indices;

    explicit Mesh(MeshFactoryContext meshFactoryContext);         //This maby someday should intake a interface of materials
    size_t calculateContentHash() const override;
    [[nodiscard]] AssetType getType() const override;
};
}


#endif //OPENGLGP_MESH_H
