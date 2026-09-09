//
// Created by redkc on 05/08/2025.
//

#include "RenderSystem.h"

#include "Asset.hpp"
#include "PlatformInterface.hpp"
#include "assetDatas/ModelData.h"
#include "systems/transformSystem/componets/TransformComponent.hpp"
#include "componets/CameraComponent.hpp"
#include "componets/MeshComponent.hpp"
#include "ecs/Scene.h"
#include "systems/editorSystem/EditorSystem.hpp"
#include "systems/gizmoSystem/GizmoSystem.hpp"

void engine::ecs::RenderSystem::Update(float deltaTime)
{
    if (scene->engine.minimized)
        return;

    auto rendererArray = scene->GetComponentArray<RendererComponent>().get();
    auto& renderers = rendererArray->GetComponents();

    auto lightArray = scene->GetComponentArray<LightComponent>().get();
    auto& lights = lightArray->GetComponents();

    auto& transforms = scene->GetIntegralComponentArray<TransformComponent>().get()->GetComponents();

    CameraObject cameraObject = scene->GetActiveCamera();

    auto editorSystem = scene->GetSystem<EditorSystem>();
    bool inEditMode = editorSystem->inEditMode;

    int width, height;
#ifdef ENABLE_IMGUI
    glm::uvec2 extent = scene->engine.graphicsEngine->getExtent();
    width = extent.x;
    height = extent.y;
#else
    scene->engine.platform->GetWindowSize(width, height);
#endif
    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

    int activeCameraCount = 0;

    if (inEditMode) {
        // Camera 0 is always the editor camera
        updateViewMatrix(editorSystem->camera, editorSystem->cameraTransform.globalMatrix);
        editorSystem->camera.aspectRatio = aspectRatio;
        updateProjectionMatrix(editorSystem->camera);
        scene->engine.graphicsEngine->setCameraData(0, editorSystem->camera.projection, editorSystem->camera.view,
                                                    editorSystem->cameraTransform.position);
        if (editorSystem->camera.skyboxMaterialId != boost::uuids::nil_uuid()) {
            scene->engine.graphicsEngine->drawSkybox(0, editorSystem->camera.skyboxMaterialId, boost::uuids::nil_uuid());
        }
        activeCameraCount = 1;

        // Camera 1 is the active scene camera (if any)
        auto& cameras = scene->GetComponentArray<CameraComponent>().get()->GetComponents();
        auto& cameraTransforms = scene->GetIntegralComponentArray<TransformComponent>().get()->GetComponents();
        for (int i = 0; i < scene->GetComponentArray<CameraComponent>().get()->GetArraySize(); i++) {
            if (scene->GetComponentArray<CameraComponent>().get()->IsComponentActive(i) && cameras[i].active) {
                auto cameraEntity = scene->GetComponentArray<CameraComponent>().get()->ComponentIndexToEntity(i);
                updateViewMatrix(cameras[i], cameraTransforms[cameraEntity].globalMatrix);
                cameras[i].aspectRatio = aspectRatio;
                updateProjectionMatrix(cameras[i]);
                scene->engine.graphicsEngine->setCameraData(1, cameras[i].projection, cameras[i].view,
                                                            cameraTransforms[cameraEntity].position);
                if (cameras[i].skyboxMaterialId != boost::uuids::nil_uuid()) {
                    scene->engine.graphicsEngine->drawSkybox(1, cameras[i].skyboxMaterialId, boost::uuids::nil_uuid());
                }
                activeCameraCount = 2;
                break;
            }
        }
    } else {
        updateViewMatrix(*cameraObject.camera, cameraObject.transform->globalMatrix);
        cameraObject.camera->aspectRatio = aspectRatio;
        updateProjectionMatrix(*cameraObject.camera);

        scene->engine.graphicsEngine->setCameraData(0, cameraObject.camera->projection, cameraObject.camera->view,
                                                    cameraObject.transform->position);

        if (cameraObject.camera->skyboxMaterialId != boost::uuids::nil_uuid()) {
            scene->engine.graphicsEngine->drawSkybox(0, cameraObject.camera->skyboxMaterialId, boost::uuids::nil_uuid());
        }
        activeCameraCount = 1;
    }

    scene->engine.graphicsEngine->setActiveCameraCount(activeCameraCount);

    // Only iterate up to the actual size of used components
    for (ComponentID i = 0; i < rendererArray->GetArraySize(); i++)
    {
        if (rendererArray->IsComponentActive(i))
        {
            Entity entity = rendererArray->ComponentIndexToEntity(i);
            if (scene->HasComponent<MeshComponent>(entity))
            {
                auto& mesh = scene->GetComponent<MeshComponent>(entity);
                if (mesh.modelUuid != boost::uuids::nil_uuid())
                {
                    for (int camIdx = 0; camIdx < activeCameraCount; ++camIdx) {
                        boost::uuids::uuid currentShader = renderers[i].shaderUuid;
                        if (inEditMode && camIdx == 0) { // Only override for the editor camera
                            if (editorSystem->currentShaderOverride == EditorSystem::ShaderOverrideMode::Wiremesh) {
                                currentShader = editorSystem->wiremeshShaderId;
                            } else if (editorSystem->currentShaderOverride == EditorSystem::ShaderOverrideMode::TexturedWiremesh) {
                                currentShader = editorSystem->wiremeshTexturedShaderId;
                            }
                        }

                        scene->engine.graphicsEngine->drawModel(camIdx, mesh.modelUuid, currentShader,
                                                                transforms[entity].globalMatrix);
                    }
                }
            }
        }
    }

    auto gizmoSystem = scene->GetSystem<GizmoSystem>();
    if (gizmoSystem != nullptr)
    {
        for (auto& command : gizmoSystem->gizmoRenderCommandQueue) {
            boost::uuids::uuid modelUuid = gizmoSystem->ModelUUIDByGizmoType(command.type);
            boost::uuids::uuid shaderUuid = gizmoSystem->ShaderUUIDByGizmoType(command.type);
            for (int camIdx = 0; camIdx < activeCameraCount; ++camIdx) {
                scene->engine.graphicsEngine->drawModel(camIdx, modelUuid, shaderUuid,
                                                        command.transform);
            }
        }
        //gizmoSystem->gizmoRenderCommandQueue.clear();
    }

    // Only iterate up to the actual size of used components
    for (ComponentID i = 0; i < lightArray->GetArraySize(); i++)
    {
        if (lightArray->IsComponentActive(i))
        {
            Entity entity = lightArray->ComponentIndexToEntity(i);
            auto& lightComponent = lights[i];

            switch (lightComponent.getType())
            {
                case LightComponent::Type::Point:
                {
                    const auto& pointData = std::get<PointLightData>(lightComponent.data);
                    gfx::PointLightData lightData{
                            lightComponent.hasShadow,
                            lightComponent.intensity,
                            lightComponent.color,
                            pointData.radius,
                            pointData.falloff,
                            pointData.shadowBias,
                            pointData.shadowStrength
                    };
                    scene->engine.graphicsEngine->drawLight(lightData, transforms[entity].globalMatrix);
                    break;
                }
                case LightComponent::Type::Spot:
                {
                    const auto& spotData = std::get<SpotLightData>(lightComponent.data);
                    gfx::SpotLightData lightData{
                            lightComponent.hasShadow,
                            lightComponent.intensity,
                            spotData.innerAngle,
                            lightComponent.color,
                            spotData.outerAngle,
                            spotData.range,
                            spotData.shadowBias,
                            spotData.shadowStrength
                    };
                    scene->engine.graphicsEngine->drawLight(lightData, transforms[entity].globalMatrix);
                    break;
                }
                case LightComponent::Type::Directional:
                {
                    const auto& dirData = std::get<DirectionalLightData>(lightComponent.data);
                    gfx::DirectionalLightData lightData{
                            lightComponent.hasShadow,
                            lightComponent.intensity,
                            lightComponent.color,
                            dirData.shadowBias,
                            dirData.shadowStrength
                    };
                    scene->engine.graphicsEngine->drawLight(lightData, transforms[entity].globalMatrix);
                    break;
                }
                default:
                    break;
            }
        }
    }
}

void RenderSystem::OnComponentAdded(ComponentID componentID, std::type_index type)
{
}
