//
// Created by redkc on 02/12/2023.
//

#include "ModelAsset.h"
#include "../src/AssimpGLMHelpers.h"

namespace am
{
    //private:
    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void ModelAsset::loadFromFile(AssetFactoryData base_factory_context)
    {
        AssetManager& assetManager = AssetManager::getInstance();
        Assimp::Importer& importer = assetManager.importer;
        const aiScene* scene = importer.ReadFile(base_factory_context.path,
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
       data.rootNode = processNode(base_factory_context, scene->mRootNode, scene);
       importer.FreeScene();
    }

    int ModelAsset::getMeshIndexInScene(const aiScene* scene, const aiMesh* targetMesh)
    {
        for (unsigned int i = 0; i < scene->mNumMeshes; i++)
        {
            if (scene->mMeshes[i] == targetMesh)
            {
                return i;
            }
        }
        return -1; // Mesh not found in scene
    }

    size_t ModelAsset::calculateContentHash() const
    {
        size_t hash = 0;

        hash = CalculateContentHash(data.rootNode);

        return hash;
    }

    AssetType ModelAsset::getType() const
    {
        return AssetType::Model;
    }


    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    Node ModelAsset::processNode(AssetFactoryData base_factory_context, aiNode* aiNode, const aiScene* scene)
    {
        Node node = {};
        node.mName = aiNode->mName.C_Str();
        node.mTransformation = ConvertMatrixToGLMFormat(aiNode->mTransformation);
        // process each mesh located at the current node
        std::vector<std::shared_ptr<AssetInfo>> meshes;
        for (unsigned int i = 0; i < aiNode->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene.
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* aiMesh = scene->mMeshes[aiNode->mMeshes[i]];
            std::shared_ptr<AssetInfo> mesh = processMesh(base_factory_context, aiMesh, scene);
            meshes.push_back(mesh);
        }
        node.meshes = meshes;
        // after we've processed all the meshes (if any) we then recursively process each of the children nodes
        for (unsigned int i = 0; i < aiNode->mNumChildren; i++)
        {
            node.mChildren.push_back(processNode(base_factory_context, aiNode->mChildren[i], scene));
        }
        return node;
    }

    std::shared_ptr<AssetInfo> ModelAsset::processMesh(AssetFactoryData baseFactoryContext, aiMesh* mesh,
                                                  const aiScene* scene)
    {
        AssetManager& assetManager = AssetManager::getInstance();
        AssetFactoryData meshFactoryContext{baseFactoryContext};
        meshFactoryContext.assetType = AssetType::Mesh;
        meshFactoryContext.assimpIndex = getMeshIndexInScene(scene, mesh);
        return assetManager.registerAsset(&meshFactoryContext).value();
    }
}
