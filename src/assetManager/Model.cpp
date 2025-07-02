//
// Created by redkc on 02/12/2023.
//

#include "Model.h"

namespace am {
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
                hash ^= mesh->contentHash + 0x9e3779b9 + (hash << 6) + (hash >> 2);
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
            meshes.push_back(processMesh(base_factory_context, mesh, scene));
        }
        // after we've processed all the meshes (if any) we then recursively process each of the children nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(base_factory_context, node->mChildren[i], scene);
        }
    }

    std::shared_ptr<AssetInfo> Model::processMesh(AssetFactoryData baseFactoryContext, aiMesh *mesh,
                                                  const aiScene *scene) {
        AssetFactoryData meshFactoryContext{baseFactoryContext};
        meshFactoryContext.assetType = AssetType::Mesh;
        meshFactoryContext.scene = scene;
        meshFactoryContext.assimpIndex = getMeshIndexInScene(scene, mesh);
        return baseFactoryContext.assetManager.registerAsset(&meshFactoryContext).value();
    }
}