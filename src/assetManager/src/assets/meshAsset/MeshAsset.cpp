//
// Created by redkc on 02/12/2023.
//

#include "MeshAsset.h"



#ifndef MESH_H
#define MESH_H


using namespace std;

namespace am {

    size_t Mesh::calculateContentHash() const {
        size_t hash = 0;

        // Hash vertices
        for (const auto &vertex: data.vertices) {
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
        for (const auto &index: data.indices) {
            hash ^= std::hash<unsigned int>{}(index) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }

        return hash;
    }


    Mesh::Mesh(AssetFactoryData meshFactoryContext): Asset(meshFactoryContext)
    {
        AssetManager &assetManager = AssetManager::getInstance();
        auto scene = assetManager.importer.GetScene();

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            scene = assetManager.importer.ReadFile(meshFactoryContext.path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
            {
                spdlog::error("Assimp error: " + string(assetManager.importer.GetErrorString()));
                throw std::runtime_error("Assimp error: " + string(assetManager.importer.GetErrorString()));
            }
        }
             // walk through each of the mesh's vertices
        auto mesh = scene->mMeshes[meshFactoryContext.assimpIndex];

        AssetFactoryData materialFactoryContext{meshFactoryContext};
        materialFactoryContext.assimpIndex = mesh->mMaterialIndex;
        materialFactoryContext.assetType = AssetType::Material;

        auto rMaterial = assetManager.registerAsset(&materialFactoryContext);
        if (!rMaterial) {
            spdlog::error("Failed to load material for mesh: " + meshFactoryContext.path);
            throw std::runtime_error("Failed to load material for mesh: " + meshFactoryContext.path);
        }
        data.material = rMaterial.value();

        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            VertexHandle vertex;
            glm::vec3 vector;

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
            if (mesh->mTextureCoords[0]) {
                glm::vec2 vec;
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
            } else {
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            }

            // vertex colors
            if (mesh->HasVertexColors(0)) { // Check if the vertex has color channel 0
                vertex.Color.r = mesh->mColors[0][i].r;
                vertex.Color.g = mesh->mColors[0][i].g;
                vertex.Color.b = mesh->mColors[0][i].b;
                vertex.Color.a = mesh->mColors[0][i].a;
            } else {
                vertex.Color = glm::vec4(1.0f); // Default to white color if no color is available
            }

            data.vertices.push_back(vertex);
        }
        // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                data.indices.push_back(face.mIndices[j]);
        }

        for (int i = 0; i < data.vertices.size(); ++i) {
            Normalize(data.vertices[i]);
        }
    }

    AssetType Mesh::getType() const {
        return AssetType::Mesh;
    }
}


#endif