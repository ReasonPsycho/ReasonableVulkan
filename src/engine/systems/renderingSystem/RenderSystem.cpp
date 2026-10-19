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
#include "componets/ThumbnailComponent.hpp"
#include "ecs/Scene.h"
#include "systems/editorSystem/EditorSystem.hpp"
#include "systems/gizmoSystem/GizmoSystem.hpp"
#include "systems/tacticalGridSystem/TacticalGridSystem.hpp"

void engine::ecs::RenderSystem::Update(float deltaTime)
{
    if (scene->engine.minimized || scene->engine.graphicsEngine == nullptr)
        return;

    CameraObject cameraObject = scene->GetActiveCamera();

    auto editorSystem = scene->engine.GetEditorSystem();
    bool inEditMode = editorSystem ? editorSystem->inEditMode : false;

    int width, height;
#ifdef ENABLE_IMGUI
    glm::uvec2 extent = scene->engine.graphicsEngine->getExtent();
    width = extent.x;
    height = extent.y;
#else
    scene->engine.platform->GetWindowSize(width, height);
#endif
    float aspectRatio = (height > 0) ? (static_cast<float>(width) / static_cast<float>(height)) : 1.0f;

    auto activeScenes = scene->engine.GetActiveScenes();
    uint32_t sceneIndex = 0;
    for (size_t s = 0; s < activeScenes.size(); ++s) {
        if (activeScenes[s].get() == scene) {
            sceneIndex = static_cast<uint32_t>(s);
            break;
        }
    }

    std::vector<uint32_t> targetCameraIndices;
    auto thumbCompArray = scene->GetComponentArray<ThumbnailComponent>();
    bool isThumbnailScene = (scene->GetSceneType() == ecs::SceneType::Thumbnail) || (thumbCompArray && thumbCompArray->GetArraySize() > 0);

    for (auto [entity, thumb] : scene->GetActiveView<ThumbnailComponent>()) {
        thumb.framesRendered++;
    }

    auto submitCameraSkybox = [&](uint32_t camIdx, ecs::CameraComponent& cam) {
        if (cam.isDirty) {
            if (auto* imgData = std::get_if<ecs::SkyboxImageData>(&cam.skyboxData)) {
                imgData->runtimeSkyboxMaterialHandle = gfx::MaterialHandle::invalid();
            }
        }
        if (cam.getSkyboxType() == ecs::CameraComponent::SkyboxType::Image) {
            if (auto* imgData = std::get_if<ecs::SkyboxImageData>(&cam.skyboxData)) {
                if (imgData->skyboxMaterialId != boost::uuids::nil_uuid()) {
                    if (!imgData->runtimeSkyboxMaterialHandle.isValid()) {
                        imgData->runtimeSkyboxMaterialHandle = scene->engine.graphicsEngine->loadMaterial(imgData->skyboxMaterialId);
                    }
                    scene->engine.graphicsEngine->drawSkybox(camIdx, imgData->runtimeSkyboxMaterialHandle, gfx::ShaderProgramHandle::invalid());
                }
            }
        } else if (cam.getSkyboxType() == ecs::CameraComponent::SkyboxType::Color) {
            if (auto* colData = std::get_if<ecs::SkyboxColorData>(&cam.skyboxData)) {
                scene->engine.graphicsEngine->drawSkybox(camIdx, colData->color);
            }
        }
    };

    if (inEditMode && editorSystem && !isThumbnailScene) {
        auto& camState = editorSystem->GetEditorCameraState(scene->GetName());
        // Editor camera for this specific scene at camera index: sceneIndex
        updateViewMatrix(camState.camera, camState.cameraTransform.globalMatrix);
        float editorAspect = (camState.lastViewportSize.x > 0.0f && camState.lastViewportSize.y > 0.0f)
            ? (camState.lastViewportSize.x / camState.lastViewportSize.y)
            : aspectRatio;
        camState.camera.aspectRatio = editorAspect;
        updateProjectionMatrix(camState.camera);
        scene->engine.graphicsEngine->setCameraData(sceneIndex, camState.camera.projection, camState.camera.view,
                                                    camState.cameraTransform.position);
        submitCameraSkybox(sceneIndex, camState.camera);
        targetCameraIndices.push_back(sceneIndex);

        // Check if this scene is the top editable active scene
        auto topEditableScene = scene->engine.GetTopEditableScene();
        bool isTopEditable = (topEditableScene && topEditableScene.get() == scene);

        uint32_t gameCameraIndex = static_cast<uint32_t>(activeScenes.size());
        uint32_t activeCamCount = static_cast<uint32_t>(activeScenes.size());

        if (isTopEditable) {
            for (auto [cameraEntity, camera, cameraTransform] : scene->GetActiveView<CameraComponent, TransformComponent>()) {
                if (camera.active) {
                    updateViewMatrix(camera, cameraTransform.globalMatrix);
                    if (camera.aspectRatio <= 0.0f) {
                        camera.aspectRatio = aspectRatio;
                    }
                    updateProjectionMatrix(camera);
                    scene->engine.graphicsEngine->setCameraData(gameCameraIndex, camera.projection, camera.view,
                                                                cameraTransform.position);
                    submitCameraSkybox(gameCameraIndex, camera);
                    targetCameraIndices.push_back(gameCameraIndex);
                    activeCamCount = gameCameraIndex + 1;
                    break;
                }
            }
        }
        scene->engine.graphicsEngine->setActiveCameraCount(activeCamCount);
    } else {
        uint32_t sceneCameraIndex = sceneIndex;
        if (cameraObject.camera && cameraObject.transform) {
            updateViewMatrix(*cameraObject.camera, cameraObject.transform->globalMatrix);
            cameraObject.camera->aspectRatio = isThumbnailScene ? 1.0f : aspectRatio;
            updateProjectionMatrix(*cameraObject.camera);

            scene->engine.graphicsEngine->setCameraData(sceneCameraIndex, cameraObject.camera->projection, cameraObject.camera->view,
                                                        cameraObject.transform->position);

            submitCameraSkybox(sceneCameraIndex, *cameraObject.camera);
            targetCameraIndices.push_back(sceneCameraIndex);
            scene->engine.graphicsEngine->setActiveCameraCount(sceneCameraIndex + 1);
        }
    }

    for (auto [entity, renderer, mesh, transform] : scene->GetActiveView<RendererComponent, MeshComponent, TransformComponent>())
    {
        if (mesh.isDirty) {
            mesh.runtimeMeshHandle = gfx::MeshHandle::invalid();
            mesh.isDirty = false;
        }

        if (renderer.isDirty) {
            renderer.runtimeShaderHandle = gfx::ShaderProgramHandle::invalid();
            renderer.runtimeMaterialHandle = gfx::MaterialHandle::invalid();
            renderer.isDirty = false;
        }

        if (mesh.meshUuid != boost::uuids::nil_uuid())
        {
            if (!mesh.runtimeMeshHandle.isValid()) {
                mesh.runtimeMeshHandle = scene->engine.graphicsEngine->loadMesh(mesh.meshUuid);
            }

            if (renderer.shaderUuid != boost::uuids::nil_uuid() && !renderer.runtimeShaderHandle.isValid()) {
                renderer.runtimeShaderHandle = scene->engine.graphicsEngine->loadShader(renderer.shaderUuid);
            }
            if (renderer.materialUuid != boost::uuids::nil_uuid() && !renderer.runtimeMaterialHandle.isValid()) {
                renderer.runtimeMaterialHandle = scene->engine.graphicsEngine->loadMaterial(renderer.materialUuid);
            }

            for (uint32_t camIdx : targetCameraIndices) {
                gfx::ShaderProgramHandle currentShader = renderer.runtimeShaderHandle;
                if (inEditMode && camIdx < activeScenes.size() && editorSystem && !isThumbnailScene) { // Only override for editor cameras
                    if (editorSystem->currentShaderOverride == EditorSystem::ShaderOverrideMode::Wiremesh) {
                        currentShader = editorSystem->wiremeshShaderHandle;
                    } else if (editorSystem->currentShaderOverride == EditorSystem::ShaderOverrideMode::TexturedWiremesh) {
                        currentShader = editorSystem->wiremeshTexturedShaderHandle;
                    }
                }

                scene->engine.graphicsEngine->drawModel(camIdx, mesh.runtimeMeshHandle, renderer.runtimeMaterialHandle, currentShader,
                                                        transform.globalMatrix);
            }
        }
    }

    auto gizmoSystem = scene->GetSystem<GizmoSystem>();
    if (gizmoSystem != nullptr && !isThumbnailScene)
    {
        for (auto& command : gizmoSystem->gizmoRenderCommandQueue) {
            gfx::MeshHandle meshHandle = gizmoSystem->MeshHandleByGizmoType(command.type);
            gfx::ShaderProgramHandle shaderHandle = gizmoSystem->ShaderHandleByGizmoType(command.type);
            if (inEditMode) {
                scene->engine.graphicsEngine->drawModel(sceneIndex, meshHandle, gfx::MaterialHandle::invalid(), shaderHandle,
                                                        command.transform);
            }
        }
        //gizmoSystem->gizmoRenderCommandQueue.clear();
    }

    auto tacticalGridSystem = scene->GetSystem<TacticalGridSystem>();
    if (tacticalGridSystem != nullptr && tacticalGridSystem->IsOverlayActive() && !isThumbnailScene)
    {
        gfx::MeshHandle decalMesh = tacticalGridSystem->GetDecalMeshHandle();
        gfx::ShaderProgramHandle gridShader = tacticalGridSystem->GetGridShaderHandle();
        const auto& cellTransforms = tacticalGridSystem->GetReachableCellTransforms();

        if (decalMesh.isValid() && gridShader.isValid()) {
            for (uint32_t camIdx : targetCameraIndices) {
                if (!cellTransforms.empty()) {
                    for (const auto& cellTransform : cellTransforms) {
                        scene->engine.graphicsEngine->drawModel(camIdx, decalMesh, gfx::MaterialHandle::invalid(), gridShader,
                                                                cellTransform);
                    }
                } else {
                    glm::mat4 decalTransform = tacticalGridSystem->GetDecalModelMatrix();
                    scene->engine.graphicsEngine->drawModel(camIdx, decalMesh, gfx::MaterialHandle::invalid(), gridShader,
                                                            decalTransform);
                }
            }
        }
    }

    for (auto [entity, lightComponent, transform] : scene->GetActiveView<LightComponent, TransformComponent>())
    {
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
                scene->engine.graphicsEngine->drawLight(lightData, transform.globalMatrix);
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
                scene->engine.graphicsEngine->drawLight(lightData, transform.globalMatrix);
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
                scene->engine.graphicsEngine->drawLight(lightData, transform.globalMatrix);
                break;
            }
            default:
                break;
        }
    }
}

