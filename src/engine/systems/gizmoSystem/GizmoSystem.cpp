//
// Created by redkc on 23/05/2026.
//

#include "GizmoSystem.hpp"

#include <Config.hpp>

#include "ecs/System.h"
#include "AssetTypes.hpp"
#include "../../../assetManager/src/assets/meshAsset/MeshAsset.h"
#include "assetDatas/ModelData.h"
#include "ecs/Scene.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace engine::ecs
{
    GizmoSystem::GizmoSystem(Scene* scene): System(scene)
    {
        Initialize();
    }

    void GizmoSystem::Initialize()
    {
        auto rayUuidOpt = scene->engine.assetManagerInterface->getAssetUuid("internalRayModel");
        if (!rayUuidOpt.has_value())
        {
            rayAssetUuid = scene->engine.assetManagerInterface->createAsset(am::AssetType::Model, std::string(RESOURCES_DIR) + "my\\internal\\internalRayModel","internalRayModel").value();
            auto rayData = scene->engine.assetManagerInterface->getAssetData<am::ModelData>(rayAssetUuid);
            auto meshData = rayData->rootNode.mChildren[0].meshes[0].get()->getAsset()->getAssetDataAs<am::MeshData>();
            am::VertexAsset v1{};
            v1.Position = glm::vec3(0.0f);
            v1.Color = glm::vec4(1.0f);

            am::VertexAsset v2{};  // Z is forward for ray
            v2.Position = glm::vec3(0.0f, 0.0f, 1.0f);
            v2.Color = glm::vec4(1.0f);

            meshData->vertices = {v1, v2};
            meshData->indices = {0, 1};

            meshData->boundingBoxMin = glm::vec3(0.0f);
            meshData->boundingBoxMax = glm::vec3(0.0f, 0.0f, 1.0f);

            rayData->boundingBoxMin = glm::vec3(0.0f);
            rayData->boundingBoxMax = glm::vec3(0.0f, 0.0f, 1.0f);

            scene->engine.assetManagerInterface->saveAsset(rayData->rootNode.mChildren[0].meshes[0].get()->id);
        }
        else
        {
            rayAssetUuid = rayUuidOpt.value();
        }

        auto cubeUuidOpt = scene->engine.assetManagerInterface->getAssetUuid("internalCubeModel");
        if (!cubeUuidOpt.has_value())
        {
            cubeAssetUuid = scene->engine.assetManagerInterface->createAsset(am::AssetType::Model, std::string(RESOURCES_DIR) + "my\\internal\\internalCubeModel","internalCubeModel").value();
            auto cubeData = scene->engine.assetManagerInterface->getAssetData<am::ModelData>(cubeAssetUuid);
            auto meshData = cubeData->rootNode.mChildren[0].meshes[0].get()->getAsset()->getAssetDataAs<am::MeshData>();

            meshData->vertices.resize(8);
            for (int i = 0; i < 8; ++i) {
                meshData->vertices[i].Position = glm::vec3(
                    (i & 1) ? 1.0f : -1.0f,
                    (i & 2) ? 1.0f : -1.0f,
                    (i & 4) ? 1.0f : -1.0f
                );
                meshData->vertices[i].Color = glm::vec4(1.0f);
            }

            meshData->indices = {
                0, 2, 3, 0, 3, 1, // Back
                4, 5, 7, 4, 7, 6, // Front
                0, 4, 6, 0, 6, 2, // Left
                1, 3, 7, 1, 7, 5, // Right
                0, 1, 5, 0, 5, 4, // Bottom
                2, 6, 7, 2, 7, 3  // Top
            };

            meshData->boundingBoxMin = glm::vec3(-1.0f);
            meshData->boundingBoxMax = glm::vec3(1.0f);

            cubeData->boundingBoxMin = glm::vec3(-1.0f);
            cubeData->boundingBoxMax = glm::vec3(1.0f);

            scene->engine.assetManagerInterface->saveAsset(cubeData->rootNode.mChildren[0].meshes[0].get()->id);
        }
        else
        {
            cubeAssetUuid = cubeUuidOpt.value();
        }

        rayShaderUuid = scene->engine.assetManagerInterface->getAssetUuid("raycastShader").value();
        cubeShaderUuid = scene->engine.assetManagerInterface->getAssetUuid("wiremeshShader").value();
    }

    void GizmoSystem::Update(float deltaTime)
    {
        for (auto it = gizmoRenderCommandQueue.begin(); it != gizmoRenderCommandQueue.end(); ) {
            if (it->isNew) {
                it->isNew = false;
                ++it;
            } else if (it->lifetime > 0) {
                it->lifetime -= deltaTime;
                if (it->lifetime <= 0) {
                    it = gizmoRenderCommandQueue.erase(it);
                } else {
                    ++it;
                }
            } else {
                it = gizmoRenderCommandQueue.erase(it);
            }
        }
    }

    void GizmoSystem::DrawRay(glm::vec3 startPos, glm::vec3 endPos, glm::vec3 color)
    {
        DrawRay(startPos, endPos, color, 0.0f);
    }

    void GizmoSystem::DrawRay(glm::vec3 startPos, glm::vec3 endPos, glm::vec3 color, float duration)
    {
        glm::vec3 dir = endPos - startPos;
        float len = glm::length(dir);
        if (len < 0.0001f) return;
        glm::vec3 z = dir / len;
        glm::vec3 up = glm::abs(z.y) < 0.999f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
        glm::vec3 x = glm::normalize(glm::cross(up, z));
        glm::vec3 y = glm::cross(z, x);

        glm::mat4 transform(1.0f);
        transform[0] = glm::vec4(x, 0.0f);
        transform[1] = glm::vec4(y, 0.0f);
        transform[2] = glm::vec4(z * len, 0.0f);
        transform[3] = glm::vec4(startPos, 1.0f);

        gizmoRenderCommandQueue.push_back({RAY, transform, color, duration, true});
    }

    void GizmoSystem::DrawCube(glm::vec3 pos, glm::vec3 scale, glm::vec3 color)
    {
        DrawCube(pos, scale, color, 0.0f);
    }

    void GizmoSystem::DrawCube(glm::vec3 pos, glm::vec3 scale, glm::vec3 color, float duration)
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), pos);
        transform = glm::scale(transform, scale);
        gizmoRenderCommandQueue.push_back({CUBE, transform, color, duration, true});
    }

    boost::uuids::uuid GizmoSystem::ModelUUIDByGizmoType(GizmoType type)
    {
        switch (type)
        {
            case RAY:
                return rayAssetUuid;
            case CUBE:
                return cubeAssetUuid;
            default:
                return boost::uuids::nil_uuid();
        }
    }

    boost::uuids::uuid GizmoSystem::ShaderUUIDByGizmoType(GizmoType type)
    {
        switch (type)
        {
        case RAY:
            return rayShaderUuid;
        case CUBE:
            return cubeShaderUuid;
        default:
            return boost::uuids::nil_uuid();
        }
    }
} // engine::ecs