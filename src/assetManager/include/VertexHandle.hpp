//
// Created by redkc on 16.05.2025.
//

#ifndef VERTEX_HPP
#define VERTEX_HPP
#define MAX_BONE_INFLUENCE 4


#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace am {
    struct VertexHandle {
        // position
        glm::vec3 Position;
        // normal
        glm::vec3 Normal;
        // texCoords
        glm::vec2 TexCoords;
        // color
        glm::vec4 Color; // Add for vertex color support
        // tangent
        glm::vec3 Tangent;
        // bitangent
        glm::vec3 Bitangent;
    };


    void Normalize(VertexHandle &vertex);
}


#endif //VERTEX_HPP
