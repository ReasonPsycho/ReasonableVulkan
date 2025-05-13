//
// Created by redkc on 02/12/2023.
//

#include "Mesh.h"

#ifndef MESH_H
#define MESH_H


using namespace std;

namespace ae {
    // constructor
    Mesh::Mesh(MeshFactoryContext meshFactoryContext) : Asset(meshFactoryContext),
                                                        vertices(meshFactoryContext.vertices),
                                                        indices(meshFactoryContext.indices) {
        // now that we have all the required data, set the vertex buffers and its attribute pointers.
    }

    size_t Mesh::calculateContentHash() const {
        size_t hash = 0;

        // Hash vertices
        for (const auto &vertex: vertices) {
            // Combine hash with vertex data
            hash ^= std::hash<float>{}(vertex.Position.x) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<float>{}(vertex.Position.y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<float>{}(vertex.Position.z) + 0x9e3779b9 + (hash << 6) + (hash >> 2);

            // Hash normal if present
            hash ^= std::hash<float>{}(vertex.Normal.x) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<float>{}(vertex.Normal.y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<float>{}(vertex.Normal.z) + 0x9e3779b9 + (hash << 6) + (hash >> 2);

            // Hash texture coordinates if present
            hash ^= std::hash<float>{}(vertex.TexCoords.x) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<float>{}(vertex.TexCoords.y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }

        // Hash indices
        for (const auto &index: indices) {
            hash ^= std::hash<unsigned int>{}(index) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }

        return hash;
    }

    AssetType Mesh::getType() const {
        return AssetType::Mesh;
    }
}


#endif
