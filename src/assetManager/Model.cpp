//
// Created by redkc on 02/12/2023.
//

#include "Model.h"

namespace ae {
    //private:
    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void Model::loadFromFile(AssetFactoryData base_factory_context) {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(base_factory_context.path,
                                                 aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                                                 // aiProcess_FlipUVs | 
                                                 aiProcess_CalcTangentSpace);
        // check for errors
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            spdlog::error("Assimp error: " + string(importer.GetErrorString()));
            return;
        }

        // process ASSIMP's root node recursively
        processNode(base_factory_context, scene->mRootNode, scene);
    }

    int Model::getMeshIndexInScene(const aiScene *scene, const aiMesh *targetMesh) {
        for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
            if (scene->mMeshes[i] == targetMesh) {
                return i;
            }
        }
        return -1; // Mesh not found in scene
    }

    size_t Model::calculateContentHash() const {
        size_t hash = 0;

        for (const auto &mesh: meshes) {
            if (mesh) {
                hash ^= mesh->calculateContentHash() + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            }
        }

        return hash;
    }

    AssetType Model::getType() const {
        return AssetType::Model;
    }


    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void Model::processNode(AssetFactoryData base_factory_context, aiNode *node, const aiScene *scene) {
        // process each mesh located at the current node
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            // the node object only contains indices to index the actual objects in the scene. 
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            AssetFactoryData meshFactoryContext{base_factory_context};
            meshFactoryContext.assetType = AssetType::Mesh;
            meshFactoryContext.assimpIndex = getMeshIndexInScene(scene, mesh);
            meshes.push_back(processMesh(base_factory_context, scene));
        }
        // after we've processed all the meshes (if any) we then recursively process each of the children nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(base_factory_context, node->mChildren[i], scene);
        }
    }

    std::shared_ptr<Mesh> Model::processMesh(AssetFactoryData baseFactoryContext, const aiScene *scene) { //TODO move this to a mesh so it can deal with it on it;s own
        vector<shared_ptr<Texture> > textures;
        
        return baseFactoryContext.assetManager.getByUUID<Mesh>(
            baseFactoryContext.assetManager.registerAsset(baseFactoryContext));
    }


    void Model::SetVertexBoneDataToDefault(Vertex &vertex) {
        for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
            vertex.m_BoneIDs[i] = -1;
            vertex.m_Weights[i] = 0.0f;
        }
    }
    

    void Model::ExtractBoneWeightForVertices(vector<Vertex> &vertices, aiMesh *mesh, const aiScene *scene) {
        auto &boneInfoMap = m_BoneInfoMap;
        int &boneCount = m_BoneCounter;

        for (int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
            int boneID = -1;
            std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();
            if (boneInfoMap.find(boneName) == boneInfoMap.end()) {
                BoneInfo newBoneInfo;
                newBoneInfo.id = boneCount;
                newBoneInfo.offset = AssimpGLMHelpers::ConvertMatrixToGLMFormat(mesh->mBones[boneIndex]->mOffsetMatrix);
                if (mesh->mBones[boneIndex]->mNode != NULL && mesh->mBones[boneIndex]->mNode->mParent) {
                    newBoneInfo.parentNode = mesh->mBones[boneIndex]->mNode->mParent->mName.C_Str();
                } else {
                    newBoneInfo.parentNode = "";
                }
                boneInfoMap[boneName] = newBoneInfo;
                boneID = boneCount;
                boneCount++;
            } else {
                boneID = boneInfoMap[boneName].id;
            }
            assert(boneID != -1);
            auto weights = mesh->mBones[boneIndex]->mWeights;
            int numWeights = mesh->mBones[boneIndex]->mNumWeights;

            for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex) {
                int vertexId = weights[weightIndex].mVertexId;
                float weight = weights[weightIndex].mWeight;
                assert(vertexId <= vertices.size());
                SetVertexBoneData(vertices[vertexId], boneID, weight);
            }
        }
    }

    void Model::SetVertexBoneData(Vertex &vertex, int boneID, float weight) {
        for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
            if (vertex.m_BoneIDs[i] < 0) {
                vertex.m_Weights[i] = weight;
                vertex.m_BoneIDs[i] = boneID;
                break;
            }
        }
    }

    void Model::Normalize(Vertex &vertex) {
        float sumOfWeights = 0;
        for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
            if (vertex.m_BoneIDs[i] != -1) {
                sumOfWeights += vertex.m_Weights[i];
            }
        }

        if (sumOfWeights == 0) {
            vertex.m_Weights[0] = 1;
        }

        // Ensure sumOfWeights is not zero to avoid division by zero
        if (sumOfWeights != 0) {
            for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
                if (vertex.m_BoneIDs[i] != -1) {
                    vertex.m_Weights[i] /= sumOfWeights;
                }
            }
        }
    }
}