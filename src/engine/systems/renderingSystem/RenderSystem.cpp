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
    if (scene->engine.minimized || scene->engine.graphicsEngine == nullptr)
        return;

    auto rendererArray = scene->GetComponentArray<RendererComponent>().get();
    auto& renderers = rendererArray->GetComponents();

    auto lightArray = scene->GetComponentArray<LightComponent>().get();
    auto& lights = lightArray->GetComponents();

    auto& transforms = scene->GetIntegralComponentArray<TransformComponent>().get()->GetComponents();

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

    if (inEditMode && editorSystem) {
        auto& camState = editorSystem->GetEditorCameraState(scene->GetName());
        // Editor camera for this specific scene at camera index: sceneIndex
        updateViewMatrix(camState.camera, camState.cameraTransform.globalMatrix);
        camState.camera.aspectRatio = aspectRatio;
        updateProjectionMatrix(camState.camera);
        scene->engine.graphicsEngine->setCameraData(sceneIndex, camState.camera.projection, camState.camera.view,
                                                    camState.cameraTransform.position);
        if (camState.camera.skyboxMaterialId != boost::uuids::nil_uuid()) {
            if (!camState.camera.runtimeSkyboxMaterialHandle.isValid()) {
                camState.camera.runtimeSkyboxMaterialHandle = scene->engine.graphicsEngine->loadMaterial(camState.camera.skyboxMaterialId);
            }
            scene->engine.graphicsEngine->drawSkybox(sceneIndex, camState.camera.runtimeSkyboxMaterialHandle, gfx::ShaderProgramHandle::invalid());
        }
        targetCameraIndices.push_back(sceneIndex);

        // Check if this scene is the top editable active scene
        auto topEditableScene = scene->engine.GetTopEditableScene();
        bool isTopEditable = (topEditableScene && topEditableScene.get() == scene);

        uint32_t gameCameraIndex = static_cast<uint32_t>(activeScenes.size());
        uint32_t activeCamCount = static_cast<uint32_t>(activeScenes.size());

        if (isTopEditable) {
            auto cameraCompArray = scene->GetComponentArray<CameraComponent>();
            if (cameraCompArray) {
                auto& cameras = cameraCompArray->GetComponents();
                auto& cameraTransforms = scene->GetIntegralComponentArray<TransformComponent>().get()->GetComponents();
                for (int i = 0; i < cameraCompArray->GetArraySize(); i++) {
                    if (cameraCompArray->IsComponentActive(i) && cameras[i].active) {
                        auto cameraEntity = cameraCompArray->ComponentIndexToEntity(i);
                        updateViewMatrix(cameras[i], cameraTransforms[cameraEntity].globalMatrix);
                        cameras[i].aspectRatio = aspectRatio;
                        updateProjectionMatrix(cameras[i]);
                        scene->engine.graphicsEngine->setCameraData(gameCameraIndex, cameras[i].projection, cameras[i].view,
                                                                    cameraTransforms[cameraEntity].position);
                        if (cameras[i].skyboxMaterialId != boost::uuids::nil_uuid()) {
                            if (!cameras[i].runtimeSkyboxMaterialHandle.isValid()) {
                                cameras[i].runtimeSkyboxMaterialHandle = scene->engine.graphicsEngine->loadMaterial(cameras[i].skyboxMaterialId);
                            }
                            scene->engine.graphicsEngine->drawSkybox(gameCameraIndex, cameras[i].runtimeSkyboxMaterialHandle, gfx::ShaderProgramHandle::invalid());
                        }
                        targetCameraIndices.push_back(gameCameraIndex);
                        activeCamCount = gameCameraIndex + 1;
                        break;
                    }
                }
            }
        }
        scene->engine.graphicsEngine->setActiveCameraCount(activeCamCount);
    } else {
        uint32_t sceneCameraIndex = sceneIndex;
        updateViewMatrix(*cameraObject.camera, cameraObject.transform->globalMatrix);
        cameraObject.camera->aspectRatio = aspectRatio;
        updateProjectionMatrix(*cameraObject.camera);

        scene->engine.graphicsEngine->setCameraData(sceneCameraIndex, cameraObject.camera->projection, cameraObject.camera->view,
                                                    cameraObject.transform->position);

        if (cameraObject.camera->skyboxMaterialId != boost::uuids::nil_uuid()) {
            if (!cameraObject.camera->runtimeSkyboxMaterialHandle.isValid()) {
                cameraObject.camera->runtimeSkyboxMaterialHandle = scene->engine.graphicsEngine->loadMaterial(cameraObject.camera->skyboxMaterialId);
            }
            scene->engine.graphicsEngine->drawSkybox(sceneCameraIndex, cameraObject.camera->runtimeSkyboxMaterialHandle, gfx::ShaderProgramHandle::invalid());
        }
        targetCameraIndices.push_back(sceneCameraIndex);
        scene->engine.graphicsEngine->setActiveCameraCount(sceneCameraIndex + 1);
    }

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
                    if (!mesh.runtimeModelHandle.isValid()) {
                        mesh.runtimeModelHandle = scene->engine.graphicsEngine->loadModel(mesh.modelUuid);
                    }

                    auto& renderer = renderers[i];
                    if (renderer.shaderUuid != boost::uuids::nil_uuid() && !renderer.runtimeShaderHandle.isValid()) {
                        renderer.runtimeShaderHandle = scene->engine.graphicsEngine->loadShader(renderer.shaderUuid);
                    }
                    if (renderer.materialUuid != boost::uuids::nil_uuid() && !renderer.runtimeMaterialHandle.isValid()) {
                        renderer.runtimeMaterialHandle = scene->engine.graphicsEngine->loadMaterial(renderer.materialUuid);
                    }

                    for (uint32_t camIdx : targetCameraIndices) {
                        gfx::ShaderProgramHandle currentShader = renderer.runtimeShaderHandle;
                        if (inEditMode && camIdx < activeScenes.size() && editorSystem) { // Only override for editor cameras
                            if (editorSystem->currentShaderOverride == EditorSystem::ShaderOverrideMode::Wiremesh) {
                                currentShader = editorSystem->wiremeshShaderHandle;
                            } else if (editorSystem->currentShaderOverride == EditorSystem::ShaderOverrideMode::TexturedWiremesh) {
                                currentShader = editorSystem->wiremeshTexturedShaderHandle;
                            }
                        }

                        scene->engine.graphicsEngine->drawModel(camIdx, mesh.runtimeModelHandle, currentShader,
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
            gfx::ModelHandle modelHandle = gizmoSystem->ModelHandleByGizmoType(command.type);
            gfx::ShaderProgramHandle shaderHandle = gizmoSystem->ShaderHandleByGizmoType(command.type);
            if (inEditMode) {
                scene->engine.graphicsEngine->drawModel(sceneIndex, modelHandle, shaderHandle,
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
