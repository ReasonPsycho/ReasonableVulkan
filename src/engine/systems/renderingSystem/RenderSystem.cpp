//
// Created by redkc on 05/08/2025.
//

#include "RenderSystem.h"

#include "Asset.hpp"
#include "PlatformInterface.hpp"
#include "assetDatas/ModelData.h"
#include "systems/transformSystem/componets/TransformComponent.hpp"
#include"componets/CameraComponent.hpp"
#include "ecs/Scene.h"
#include "systems/editorSystem/EditorSystem.hpp"

void engine::ecs::RenderSystem::Update(float deltaTime)
{
    if (scene->engine.minimized)
        return;

    auto modelArray = scene->GetComponentArray<RendererComponent>().get();
    auto& models = modelArray->GetComponents();

    auto lightArray = scene->GetComponentArray<LightComponent>().get();
    auto& lights = lightArray->GetComponents();

    auto& transforms = scene->GetIntegralComponentArray<TransformComponent>().get()->GetComponents();

    // Only iterate up to the actual size of used components
    for (ComponentID i = 0; i < modelArray->GetArraySize(); i++)
    {
        if (modelArray->IsComponentActive(i))
        {
            Entity entity = modelArray->ComponentIndexToEntity(i);
            if (models[i].modelUuid != boost::uuids::nil_uuid())
            {
                scene->engine.graphicsEngine->drawModel(models[i].modelUuid, models[i].shaderUuid, transforms[entity].globalMatrix);
            }
        }
    }

    // Only iterate up to the actual size of used components
      for (ComponentID i = 0; i < lightArray->GetArraySize(); i++)
            {
                if (lightArray->IsComponentActive(i))
                {
                    Entity entity = lightArray->ComponentIndexToEntity(i);
                    auto& lightComponent = lights[i];

                    switch (lightComponent.type)
                    {
                    case LightComponent::Type::Point:
                    {
                        const auto& pointData = std::get<PointLightData>(lightComponent.data);
                        gfx::PointLightData lightData{
                            lightComponent.intensity,
                            lightComponent.color,
                            pointData.radius,
                            pointData.falloff
                        };
                        scene->engine.graphicsEngine->drawLight(lightData, transforms[entity].globalMatrix);
                        break;
                    }
                    case LightComponent::Type::Spot:
                    {
                        const auto& spotData = std::get<SpotLightData>(lightComponent.data);
                        gfx::SpotLightData lightData{
                            lightComponent.intensity,
                            spotData.innerAngle,
                            lightComponent.color,
                            spotData.outerAngle,
                            spotData.range
                        };
                        scene->engine.graphicsEngine->drawLight(lightData, transforms[entity].globalMatrix);
                        break;
                    }
                    case LightComponent::Type::Directional:
                    {
                        gfx::DirectionalLightData lightData{
                            lightComponent.intensity,
                            lightComponent.color,
                        };
                        scene->engine.graphicsEngine->drawLight(lightData, transforms[entity].globalMatrix);
                        break;
                    }
                    default:
                        break;
                    }
                }
            }

    CameraObject cameraObject = scene->GetActiveCamera();

    int width, height;
#ifdef ENABLE_IMGUI
    glm::uvec2 extent = scene->engine.graphicsEngine->getExtent();
    width = extent.x;
    height = extent.y;
#else
    scene->engine.platform->GetWindowSize(width, height);
#endif
    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

    updateViewMatrix(*cameraObject.camera, cameraObject.transform->globalMatrix);
    cameraObject.camera->aspectRatio = aspectRatio;
    updateProjectionMatrix(*cameraObject.camera);

    scene->engine.graphicsEngine->setCameraData(0, cameraObject.camera->projection, cameraObject.camera->view, cameraObject.transform->position);

    if (cameraObject.camera->skyboxMaterialId != boost::uuids::nil_uuid()) {
        scene->engine.graphicsEngine->drawSkybox(cameraObject.camera->skyboxMaterialId, boost::uuids::nil_uuid());
    }
}

void RenderSystem::OnComponentAdded(ComponentID componentID, std::type_index type)
{
  if (type == typeid(RendererComponent))
  {
      auto& model = scene->GetComponentArray<RendererComponent>().get()->GetComponent(componentID);

      if (model.modelUuid == boost::uuids::nil_uuid())
          return;

      auto modelData = scene->engine.assetManagerInterface->getAsset(model.modelUuid).value()->getAssetDataAs<am::ModelData>();
      model.boundingBoxMin = modelData->boundingBoxMin;
      model.boundingBoxMax = modelData->boundingBoxMax;
  }
}
