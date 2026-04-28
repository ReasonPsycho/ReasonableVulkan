//
// Created by redkc on 02/12/2023.
//

#include "ModelAsset.h"
#include "../src/AssimpGLMHelpers.h"
#include "../JsonHelpers.hpp"

namespace am
{
    ModelAsset::ModelAsset(const boost::uuids::uuid& id, ImportContext assetFactoryData) : Asset(id, assetFactoryData)
    {
        loadFromFile(assetFactoryData);
    }

    void ModelAsset::loadFromFile(ImportContext base_factory_context)
    {
        AssetManager& assetManager = AssetManager::getInstance();
        Assimp::Importer& importer = assetManager.importer;
        const aiScene* scene = importer.ReadFile(base_factory_context.importPath,
                                                 aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                                                 // aiProcess_FlipUVs |
                                                 aiProcess_CalcTangentSpace);
        // check for errors
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            spdlog::error("Assimp error: " + string(importer.GetErrorString()));
            return;
        }

        data.boundingBoxMin = glm::vec3(std::numeric_limits<float>::max());
        data.boundingBoxMax = glm::vec3(std::numeric_limits<float>::lowest());

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

    void ModelAsset::SaveAssetToJson(rapidjson::Document& document) {
        auto& allocator = document.GetAllocator();
        if (!document.IsObject()) {
            document.SetObject();
        }

        auto addVec3 = [&](const char* key, const glm::vec3& vec) {
            rapidjson::Value array(rapidjson::kArrayType);
            array.PushBack(vec.x, allocator);
            array.PushBack(vec.y, allocator);
            array.PushBack(vec.z, allocator);
            document.AddMember(rapidjson::StringRef(key), array, allocator);
        };

        addVec3("boundingBoxMin", data.boundingBoxMin);
        addVec3("boundingBoxMax", data.boundingBoxMax);

        std::function<rapidjson::Value(const Node&)> serializeNode = [&](const Node& node) -> rapidjson::Value {
            rapidjson::Value nodeObj(rapidjson::kObjectType);
            nodeObj.AddMember("name", rapidjson::Value(node.mName.c_str(), allocator), allocator);

            // Matrix
            rapidjson::Value matrix(rapidjson::kArrayType);
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    matrix.PushBack(node.mTransformation[i][j], allocator);
                }
            }
            nodeObj.AddMember("transformation", matrix, allocator);

            // Meshes
            rapidjson::Value meshes(rapidjson::kArrayType);
            for (const auto& mesh : node.meshes) {
                if (mesh) {
                    std::string binaryPath = mesh->jsonPath;
                    meshes.PushBack(rapidjson::Value(binaryPath.c_str(), allocator), allocator);
                }
            }
            nodeObj.AddMember("meshes", meshes, allocator);

            // Children
            rapidjson::Value children(rapidjson::kArrayType);
            for (const auto& child : node.mChildren) {
                children.PushBack(serializeNode(child), allocator);
            }
            nodeObj.AddMember("children", children, allocator);

            return nodeObj;
        };

        document.AddMember("rootNode", serializeNode(data.rootNode), allocator);
    }

    ModelAsset::ModelAsset(const boost::uuids::uuid& id, const std::string& path, AssetFormat format) : Asset(id, path, format) {
        if (format == AssetFormat::Json) {
            rapidjson::Document document;
            if (!loadJsonFromFile(path, document)) {
                spdlog::error("Failed to load ModelAsset from JSON: {}", path);
                return;
            }
            AssetManager& assetManager = AssetManager::getInstance();

            auto loadVec3 = [&](const char* key, glm::vec3& vec) {
                if (document.HasMember(key) && document[key].IsArray() && document[key].Size() == 3) {
                    vec.x = document[key][0].GetFloat();
                    vec.y = document[key][1].GetFloat();
                    vec.z = document[key][2].GetFloat();
                }
            };

            loadVec3("boundingBoxMin", data.boundingBoxMin);
            loadVec3("boundingBoxMax", data.boundingBoxMax);

            std::function<void(const rapidjson::Value&, Node&, Node*)> deserializeNode = [&](const rapidjson::Value& val, Node& node, Node* parent) {
                if (val.HasMember("name") && val["name"].IsString()) node.mName = val["name"].GetString();
                node.mParent = parent;

                if (val.HasMember("transformation") && val["transformation"].IsArray() && val["transformation"].Size() == 16) {
                    for (int i = 0; i < 4; ++i) {
                        for (int j = 0; j < 4; ++j) {
                            node.mTransformation[i][j] = val["transformation"][i * 4 + j].GetFloat();
                        }
                    }
                }

                if (val.HasMember("meshes") && val["meshes"].IsArray()) {
                    node.meshes.clear();
                    for (auto& m : val["meshes"].GetArray()) {
                        if (m.IsString()) {
                            auto result = assetManager.registerAsset(m.GetString());
                            if (result) node.meshes.push_back(assetManager.getAssetInfo(result.value()).value_or(nullptr));
                        }
                    }
                }

                if (val.HasMember("children") && val["children"].IsArray()) {
                    node.mChildren.clear();
                    for (auto& c : val["children"].GetArray()) {
                        Node child;
                        deserializeNode(c, child, &node);
                        node.mChildren.push_back(std::move(child));
                    }
                }
            };

            if (document.HasMember("rootNode") && document["rootNode"].IsObject()) {
                deserializeNode(document["rootNode"], data.rootNode, nullptr);
            }
        }
    }

    AssetType ModelAsset::getType() const
    {
        return AssetType::Model;
    }


    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    Node ModelAsset::processNode(ImportContext base_factory_context, aiNode* aiNode, const aiScene* scene)
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
            auto meshId = processMesh(base_factory_context, aiMesh, scene);
            AssetManagerInterface &assetManager = AssetManager::getInstance();
            auto meshData = assetManager.getAssetData<MeshData>(meshId);

            data.boundingBoxMin.x = std::min(data.boundingBoxMin.x,meshData->boundingBoxMin.x);
            data.boundingBoxMin.y = std::min(data.boundingBoxMin.y,meshData->boundingBoxMin.y);
            data.boundingBoxMin.z = std::min(data.boundingBoxMin.z,meshData->boundingBoxMin.z);

            data.boundingBoxMax.x = std::max(data.boundingBoxMax.x,meshData->boundingBoxMax.x);
            data.boundingBoxMax.y = std::max(data.boundingBoxMax.y,meshData->boundingBoxMax.y);
            data.boundingBoxMax.z = std::max(data.boundingBoxMax.z,meshData->boundingBoxMax.z);

            auto mesh = assetManager.getAssetInfo(meshId);
            meshes.push_back(mesh.value());
        }



        node.meshes = meshes;
        // after we've processed all the meshes (if any) we then recursively process each of the children nodes
        for (unsigned int i = 0; i < aiNode->mNumChildren; i++)
        {
            node.mChildren.push_back(processNode(base_factory_context, aiNode->mChildren[i], scene));
        }
        return node;
    }

    boost::uuids::uuid ModelAsset::processMesh(ImportContext baseFactoryContext, aiMesh* mesh,
                                               const aiScene* scene)
    {
        AssetManager& assetManager = AssetManager::getInstance();
        ImportContext meshFactoryContext{baseFactoryContext};
        meshFactoryContext.assetType = AssetType::Mesh;
        meshFactoryContext.assimpIndex = getMeshIndexInScene(scene, mesh);
        return assetManager.registerAsset(meshFactoryContext).value();
    }
}
