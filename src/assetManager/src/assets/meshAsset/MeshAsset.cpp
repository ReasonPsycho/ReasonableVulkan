//
// Created by redkc on 02/12/2023.
//

#include "MeshAsset.h"
#include "../../JsonHelpers.hpp"



#ifndef MESH_H
#define MESH_H


using namespace std;

namespace am {

    MeshAsset::MeshAsset(const boost::uuids::uuid& id) : Asset(id), importContext("", AssetType::Other) {
    }

    size_t MeshAsset::calculateContentHash() const {
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


    MeshAsset::MeshAsset(const boost::uuids::uuid& id, const ImportContext& assetFactoryData): Asset(id, assetFactoryData), importContext(assetFactoryData)
    {
        AssetManager &assetManager = AssetManager::getInstance();
        auto scene = assetManager.importer.GetScene();

        data.boundingBoxMin = glm::vec3(std::numeric_limits<float>::max());
        data.boundingBoxMax = glm::vec3(std::numeric_limits<float>::lowest());

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            scene = assetManager.importer.ReadFile(assetFactoryData.importPath, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
            {
                spdlog::error("Assimp error: " + string(assetManager.importer.GetErrorString()));
                throw std::runtime_error("Assimp error: " + string(assetManager.importer.GetErrorString()));
            }
        }
             // walk through each of the mesh's vertices
        auto mesh = scene->mMeshes[assetFactoryData.assimpIndex];

        ImportContext materialFactoryContext{assetFactoryData};
        materialFactoryContext.assimpIndex = mesh->mMaterialIndex;
        materialFactoryContext.assetType = AssetType::Material;

        auto rMaterial = assetManager.registerAsset(materialFactoryContext);
        if (!rMaterial) {
            spdlog::error("Failed to load material for mesh: " + assetFactoryData.importPath);
            throw std::runtime_error("Failed to load material for mesh: " + assetFactoryData.importPath);
        }
        data.material = assetManager.getAssetInfo(rMaterial.value()).value_or(nullptr);

        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            VertexAsset vertex;
            glm::vec3 vector;

            // positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;

            //Bounding box
            data.boundingBoxMin.x = std::min(data.boundingBoxMin.x,vector.x);
            data.boundingBoxMin.y = std::min(data.boundingBoxMin.y,vector.y);
            data.boundingBoxMin.z = std::min(data.boundingBoxMin.z,vector.z);

            data.boundingBoxMax.x = std::max(data.boundingBoxMax.x,vector.x);
            data.boundingBoxMax.y = std::max(data.boundingBoxMax.y,vector.y);
            data.boundingBoxMax.z = std::max(data.boundingBoxMax.z,vector.z);

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

        if ( std::abs(data.boundingBoxMin.x - data.boundingBoxMax.x) <= 0.0f )
        {
            data.boundingBoxMin.x -= 0.1f;
            data.boundingBoxMax.x += 0.1f;
        }

        if ( std::abs(data.boundingBoxMin.y - data.boundingBoxMax.y) <= 0.0f )
        {
            data.boundingBoxMin.y -= 0.1f;
            data.boundingBoxMax.y += 0.1f;
        }

        if ( std::abs(data.boundingBoxMin.z - data.boundingBoxMax.z) <= 0.0f )
        {
            data.boundingBoxMin.z -= 0.1f;
            data.boundingBoxMax.z += 0.1f;
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

    MeshAsset::MeshAsset(const std::string& path, AssetFormat format): Asset(path, format), importContext("",AssetType::Other)
    {
        if (format == AssetFormat::Json) {
            rapidjson::Document document;
            if (!loadJsonFromFile(path, document)) {
                spdlog::error("Failed to load MeshAsset from JSON: {}", path);
                return;
            }

            if (document.HasMember("uuid") && document["uuid"].IsString()) {
                id = boost::uuids::string_generator()(document["uuid"].GetString());
            }

            AssetManager &assetManager = AssetManager::getInstance();

            if (document.HasMember("importContext") && document["importContext"].IsObject()) {
                const auto& ic = document["importContext"];
                if (ic.HasMember("path") && ic["path"].IsString()) importContext.importPath = ic["path"].GetString();
                if (ic.HasMember("type") && ic["type"].IsString()) importContext.assetType = StringToAssetType(ic["type"].GetString());
                if (ic.HasMember("assimpIndex") && ic["assimpIndex"].IsInt()) importContext.assimpIndex = ic["assimpIndex"].GetInt();
            }

            if (document.HasMember("binPath") && document["binPath"].IsString()) {
                std::string binPath = document["binPath"].GetString();

                std::ifstream ifs(binPath, std::ios::binary | std::ios::in);
                if (!ifs.is_open()) {
                    spdlog::error("Failed to open file for reading binary asset: {}", binPath);
                    return;
                }

                // Read magic number
                char magic[6];
                ifs.read(magic, sizeof(magic));
                if (std::string(magic) != "RMESH") {
                    spdlog::error("Invalid magic number in binary mesh asset: {}", binPath);
                    return;
                }

                // Read material path
                size_t pathSize;
                ifs.read(reinterpret_cast<char*>(&pathSize), sizeof(pathSize));
                std::string materialPath(pathSize, '\0');
                ifs.read(&materialPath[0], pathSize);

                if (!materialPath.empty()) {
                    auto result = AssetManager::getInstance().registerAsset(materialPath);
                    if (result) {
                        data.material = AssetManager::getInstance().getAssetInfo(result.value()).value_or(nullptr);
                    }
                }

                // Read bounding box
                ifs.read(reinterpret_cast<char*>(&data.boundingBoxMin), sizeof(glm::vec3));
                ifs.read(reinterpret_cast<char*>(&data.boundingBoxMax), sizeof(glm::vec3));

                // Read vertices
                size_t vertexCount;
                ifs.read(reinterpret_cast<char*>(&vertexCount), sizeof(vertexCount));
                data.vertices.resize(vertexCount);
                if (vertexCount > 0) {
                    ifs.read(reinterpret_cast<char*>(data.vertices.data()), vertexCount * sizeof(am::VertexAsset));
                }

                // Read indices
                size_t indexCount;
                ifs.read(reinterpret_cast<char*>(&indexCount), sizeof(indexCount));
                data.indices.resize(indexCount);
                if (indexCount > 0) {
                    ifs.read(reinterpret_cast<char*>(data.indices.data()), indexCount * sizeof(unsigned int));
                }

                ifs.close();
            }

            if (document.HasMember("material") && document["material"].IsString()) {
                std::string materialUuidStr = document["material"].GetString();
                try {
                    boost::uuids::uuid materialId = boost::uuids::string_generator()(materialUuidStr);
                    data.material = assetManager.getAssetInfo(materialId).value_or(nullptr);
                    if (!data.material) {
                        spdlog::warn("Material asset with UUID {} not found for mesh {}", materialUuidStr, path);
                    }
                } catch (const std::exception& e) {
                    spdlog::error("Failed to parse material UUID {} for mesh {}: {}", materialUuidStr, path, e.what());
                }
            }

            auto loadVec3 = [&](const char* key, glm::vec3& vec) {
                if (document.HasMember(key) && document[key].IsArray() && document[key].Size() == 3) {
                    vec.x = document[key][0].GetFloat();
                    vec.y = document[key][1].GetFloat();
                    vec.z = document[key][2].GetFloat();
                }
            };

            loadVec3("boundingBoxMin", data.boundingBoxMin);
            loadVec3("boundingBoxMax", data.boundingBoxMax);
        } else if (format == AssetFormat::Binary) {
            std::ifstream ifs(path, std::ios::binary | std::ios::in);
            if (!ifs.is_open()) {
                spdlog::error("Failed to open file for reading binary asset: {}", path);
                return;
            }

            // Read magic number
            char magic[6];
            ifs.read(magic, sizeof(magic));
            if (std::string(magic) != "RMESH") {
                spdlog::error("Invalid magic number in binary mesh asset: {}", path);
                return;
            }

            // Read UUID
            ifs.read(reinterpret_cast<char*>(&id), 16);

            // Read material UUID
            boost::uuids::uuid materialId;
            ifs.read(reinterpret_cast<char*>(&materialId), 16);

            if (!materialId.is_nil()) {
                data.material = AssetManager::getInstance().getAssetInfo(materialId).value_or(nullptr);
                if (!data.material) {
                    spdlog::warn("Material asset with UUID {} not found for mesh {}", boost::uuids::to_string(materialId), path);
                }
            }

            // Read bounding box
            ifs.read(reinterpret_cast<char*>(&data.boundingBoxMin), sizeof(glm::vec3));
            ifs.read(reinterpret_cast<char*>(&data.boundingBoxMax), sizeof(glm::vec3));

            // Read vertices
            size_t vertexCount;
            ifs.read(reinterpret_cast<char*>(&vertexCount), sizeof(vertexCount));
            data.vertices.resize(vertexCount);
            if (vertexCount > 0) {
                ifs.read(reinterpret_cast<char*>(data.vertices.data()), vertexCount * sizeof(am::VertexAsset));
            }

            // Read indices
            size_t indexCount;
            ifs.read(reinterpret_cast<char*>(&indexCount), sizeof(indexCount));
            data.indices.resize(indexCount);
            if (indexCount > 0) {
                ifs.read(reinterpret_cast<char*>(data.indices.data()), indexCount * sizeof(unsigned int));
            }

            ifs.close();
        }
    }

    void MeshAsset::SaveAssetToJson(rapidjson::Document& document) {
        auto& allocator = document.GetAllocator();
        if (!document.IsObject()) {
            document.SetObject();
        }

        document.AddMember("uuid", rapidjson::Value(boost::uuids::to_string(id).c_str(), allocator), allocator);

        rapidjson::Value icObj(rapidjson::kObjectType);
        icObj.AddMember("path", rapidjson::Value(importContext.importPath.c_str(), allocator), allocator);
        icObj.AddMember("type", rapidjson::Value(AssetTypeToString(importContext.assetType).c_str(), allocator), allocator);
        icObj.AddMember("assimpIndex", importContext.assimpIndex, allocator);
        document.AddMember("importContext", icObj, allocator);



        if (data.material) {
            document.AddMember("material", rapidjson::Value(boost::uuids::to_string(data.material->id).c_str(), allocator), allocator);
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
    }

    AssetType MeshAsset::getType() const {
        return AssetType::Mesh;
    }

    void MeshAsset::SaveAssetToBin(std::string& path) {
        std::ofstream ofs(path, std::ios::binary | std::ios::out);
        if (!ofs.is_open()) {
            spdlog::error("Failed to open file for writing binary asset: {}", path);
            return;
        }

        // Write magic number or version if needed
        const char magic[] = "RMESH";
        ofs.write(magic, sizeof(magic));

        // Write UUID
        ofs.write(reinterpret_cast<const char*>(&id), 16);

        // Write material UUID
        boost::uuids::uuid materialId = data.material ? data.material->id : boost::uuids::nil_uuid();
        ofs.write(reinterpret_cast<const char*>(&materialId), 16);

        // Write bounding box
        ofs.write(reinterpret_cast<const char*>(&data.boundingBoxMin), sizeof(glm::vec3));
        ofs.write(reinterpret_cast<const char*>(&data.boundingBoxMax), sizeof(glm::vec3));

        // Write vertices
        size_t vertexCount = data.vertices.size();
        ofs.write(reinterpret_cast<const char*>(&vertexCount), sizeof(vertexCount));
        if (vertexCount > 0) {
            ofs.write(reinterpret_cast<const char*>(data.vertices.data()), vertexCount * sizeof(am::VertexAsset));
        }

        // Write indices
        size_t indexCount = data.indices.size();
        ofs.write(reinterpret_cast<const char*>(&indexCount), sizeof(indexCount));
        if (indexCount > 0) {
            ofs.write(reinterpret_cast<const char*>(data.indices.data()), indexCount * sizeof(unsigned int));
        }

        ofs.close();
    }
}


#endif