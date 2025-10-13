//
// Created by redkc on 05/08/2025.
//

#include "RenderSystem.h"

#include "systems/transformSystem/componets/Transform.hpp"
#include"componets/Camera.hpp"
#include "ecs/Scene.h"

void engine::ecs::RenderSystem::Update(float deltaTime)
{
    if (scene->engine.minimized)
        return;

    auto modelArray = scene->GetComponentArray<Model>().get();
    auto& models = modelArray->GetComponents();
    auto& transforms = scene->GetIntegralComponentArray<Transform>().get()->GetComponents();
    auto& cameras = scene->GetComponentArray<Camera>().get()->GetComponents();

    // Only iterate up to the actual size of used components
    for (ComponentIndex i = 0; i < modelArray->GetArraySize(); i++)
    {
        if (modelArray->IsComponentActive(modelArray->ComponentIndexToEntity(i)))
        {
            Entity entity = modelArray->ComponentIndexToEntity(i);
            if (models[i].modelUuid != boost::uuids::nil_uuid())
            {
                scene->engine.graphicsEngine->drawModel(models[i].modelUuid, transforms[entity].globalMatrix);
            }
        }
    }

    auto cameraEntity =scene->GetComponentArray<Camera>().get()->ComponentIndexToEntity(0); //This will blow up on first change in camera

    int width, height;
    scene->engine.platform->GetWindowSize(width, height);
    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

    updateViewMatrix(cameras[0], transforms[cameraEntity].globalMatrix);
    cameras[0].aspectRatio = aspectRatio;
    updateProjectionMatrix(cameras[0]);

    scene->engine.graphicsEngine->setCameraData(cameras[0].projection, cameras[0].view, cameras[0].lightpos);
    scene->engine.graphicsEngine->renderFrame();
}
