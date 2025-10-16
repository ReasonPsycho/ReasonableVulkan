//
// Created by redkc on 05/08/2025.
//

#include "RenderSystem.h"

#include "PlatformInterface.hpp"
#include "systems/transformSystem/componets/Transform.hpp"
#include"componets/Camera.hpp"
#include "ecs/Scene.h"
#include "systems/editorSystem/EditorSystem.hpp"

void engine::ecs::RenderSystem::Update(float deltaTime)
{
    if (scene->engine.minimized)
        return;

    auto modelArray = scene->GetComponentArray<Model>().get();
    auto& models = modelArray->GetComponents();
    auto& transforms = scene->GetIntegralComponentArray<Transform>().get()->GetComponents();

    // Only iterate up to the actual size of used components
    for (ComponentIndex i = 0; i < modelArray->GetArraySize(); i++)
    {
        if (modelArray->IsComponentActive(i))
        {
            Entity entity = modelArray->ComponentIndexToEntity(i);
            if (models[i].modelUuid != boost::uuids::nil_uuid())
            {
                scene->engine.graphicsEngine->drawModel(models[i].modelUuid, transforms[entity].globalMatrix);
            }
        }
    }

    CameraObject cameraObject = scene->GetActiveCamera();

    int width, height;
    scene->engine.platform->GetWindowSize(width, height);
    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

    updateViewMatrix(*cameraObject.camera, cameraObject.transform->globalMatrix);
    cameraObject.camera->aspectRatio = aspectRatio;
    updateProjectionMatrix(*cameraObject.camera);

    scene->engine.graphicsEngine->setCameraData(cameraObject.camera->projection, cameraObject.camera->view, cameraObject.camera->lightpos);
    scene->engine.graphicsEngine->renderFrame();
}
