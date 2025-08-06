//
// Created by redkc on 02/12/2023.
//

#include "Mesh.h"



#ifndef MESH_H
#define MESH_H


using namespace std;

namespace am {

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


    Mesh::Mesh(AssetFactoryData meshFactoryContext): Asset(meshFactoryContext)
    {
        auto scene = meshFactoryContext.assetManager.importer.GetScene();

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            scene = meshFactoryContext.assetManager.importer.ReadFile(meshFactoryContext.path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
            {
                spdlog::error("Assimp error: " + string(meshFactoryContext.assetManager.importer.GetErrorString()));
                throw std::runtime_error("Assimp error: " + string(meshFactoryContext.assetManager.importer.GetErrorString()));
            }
        }
             // walk through each of the mesh's vertices
        auto mesh = scene->mMeshes[meshFactoryContext.assimpIndex];

        AssetFactoryData materialFactoryContext{meshFactoryContext};
        materialFactoryContext.assimpIndex = mesh->mMaterialIndex;
        materialFactoryContext.assetType = AssetType::Material;

        auto rMaterial = materialFactoryContext.assetManager.registerAsset(&materialFactoryContext);
        if (!rMaterial) {
            spdlog::error("Failed to load material for mesh: " + meshFactoryContext.path);
            throw std::runtime_error("Failed to load material for mesh: " + meshFactoryContext.path);
        }
        material = rMaterial.value();

        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            glm::vec3 vector;
            // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
            // positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;

            // normals
            if (mesh->HasNormals()) {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }
            // texture coordinates
            if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
            {
                glm::vec2 vec;
                // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
                // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
                // tangent
                vector.x = mesh->mTangents[i].x;
                vector.y = mesh->mTangents[i].y;
                vector.z = mesh->mTangents[i].z;
                vertex.Tangent = vector;
                // bitangent
                vector.x = mesh->mBitangents[i].x;
                vector.y = mesh->mBitangents[i].y;
                vector.z = mesh->mBitangents[i].z;
                vertex.Bitangent = vector;
            } else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);
        }
        // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        for (int i = 0; i < vertices.size(); ++i) {
            Normalize(vertices[i]);
        }
    }

    AssetType Mesh::getType() const {
        return AssetType::Mesh;
    }
}


#endif
