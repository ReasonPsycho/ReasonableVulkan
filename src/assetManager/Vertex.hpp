//
// Created by redkc on 16.05.2025.
//

#ifndef VERTEX_HPP
#define VERTEX_HPP
#define MAX_BONE_INFLUENCE 4


#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace ae {
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

    void Normalize(Vertex &vertex);
}


#endif //VERTEX_HPP
