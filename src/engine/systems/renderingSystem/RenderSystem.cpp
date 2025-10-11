//
// Created by redkc on 05/08/2025.
//

#include "RenderSystem.h"

#include "systems/transformSystem/componets/Transform.h"
#include"componets/Camera.hpp"
#include "ecs/Scene.h"

void engine::ecs::RenderSystem::Update(float deltaTime)
{
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
            scene->engine.graphicsEngine->drawModel(models[i].modelUuid, transforms[entity].globalMatrix);
        }
    }

    auto cameraEntity =scene->GetComponentArray<Camera>().get()->ComponentIndexToEntity(0); //This will blow up on first change in camera
    updateViewMatrix(cameras[0],transforms[cameraEntity].globalMatrix);
    updateProjectionMatrix(cameras[0]);


    scene->engine.graphicsEngine->setCameraData(cameras[0].projection, cameras[0].view, cameras[0].lightpos);
    scene->engine.graphicsEngine->renderFrame();
}
