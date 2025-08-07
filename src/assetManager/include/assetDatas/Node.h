//
// Created by redkc on 06/08/2025.
//

#ifndef NODE_H
#define NODE_H
#include <string>
#include <glm/mat4x4.hpp>

#include "../AssetInfo.hpp"

namespace am
{
    struct Node
    {
        std::string mName;
        glm::mat4x4 mTransformation;
        Node* mParent;
        std::vector<std::shared_ptr<AssetInfo>> meshes;
        std::vector<Node> mChildren;
    };

    [[nodiscard]] inline size_t CalculateContentHash(Node node)
    {
        size_t hash = 0;
        hash ^= std::hash<std::string>{} (node.mName);
        for (int x = 0; x < 4; ++x)
        {
            for (int y = 0; y < 4; ++y)
            {
                hash ^= std::hash<float>{} (node.mTransformation[x][y]) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            }
        }

        // Hash vertices
        for (const auto &mesh: node.meshes)
        {
            // Combine hash with vertex data

            hash ^= std::hash<float>{}(mesh->contentHash) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }

        for (const auto &node: node.mChildren)
        {
            // Combine hash with vertex data

            hash ^= std::hash<float>{}(CalculateContentHash(node)) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }

        return hash;
    }
}

#endif //NODE_H
