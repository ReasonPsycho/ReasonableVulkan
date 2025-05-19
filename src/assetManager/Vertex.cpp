#include "Vertex.hpp"
#include <glm/geometric.hpp>

namespace ae {
    void Normalize(Vertex &vertex) {
        // Normalize the normal vector
        vertex.Normal = glm::normalize(vertex.Normal);

        // Normalize the tangent vector
        vertex.Tangent = glm::normalize(vertex.Tangent);

        // Normalize the bitangent vector
        vertex.Bitangent = glm::normalize(vertex.Bitangent);
    }
}
