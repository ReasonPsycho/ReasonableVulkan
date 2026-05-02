#define SDL_MAIN_HANDLED

#include <iostream>
#include <cstdlib>
#include <glm/ext/matrix_transform.hpp>
#include <SDL3/SDL.h>

#include "Asset.hpp"
#include "assetDatas/MaterialData.h"
#include "assetDatas/MeshData.h"
#include "assetDatas/ModelData.h"
#include "assetDatas/TextureData.h"
#include "platform/src/Platform.hpp"
#include "platform/include/PlatformInterface.hpp"
#include "assetManager/src/AssetManager.hpp"
#include "ecs/Scene.h"
#include "engine/Engine.h"
#include "systems/renderingSystem/componets/CameraComponent.hpp"
#include "systems/renderingSystem/componets/LightComponent.hpp"
#include "systems/renderingSystem/componets/RendererComponent.hpp"
#include "vks/VulkanRenderer.h"


namespace am
{
    struct MaterialData;
    struct TextureData;
}

int main(int argc, char *argv[]) {
    plt::PlatformInterface* platform = new plt::Platform();
    // 1. Initialize platform (SDL window, input, etc.)
    if (!platform->Init("My Game Engine", 1280, 720)) {
        return EXIT_FAILURE;
    }

    am::AssetManagerInterface& assetManager = am::AssetManager::getInstance();
    assetManager.Initialize(platform);
    /* //asset loading
    auto pbrShaderId = assetManager.registerAsset("C:/Users/redkc/CLionProjects/ReasonableVulkan/res/shaders/jsons/pbr.shaderImport","pbrShader");
    assetManager.registerAsset("C:/Users/redkc/CLionProjects/ReasonableVulkan/res/shaders/jsons/wiremesh.shaderImport","wiremeshShader");
    assetManager.registerAsset("C:/Users/redkc/CLionProjects/ReasonableVulkan/res/shaders/jsons/wiremesh_textured.shaderImport","wiremeshTexturedShader");
    assetManager.registerAsset("C:/Users/redkc/CLionProjects/ReasonableVulkan/res/shaders/jsons/skybox.shaderImport","skyboxShader");
    assetManager.registerAsset("C:/Users/redkc/CLionProjects/ReasonableVulkan/res/shaders/jsons/shadowMap.shaderImport","shadowMapShader");
    assetManager.registerAsset("C:/Users/redkc/CLionProjects/ReasonableVulkan/res/shaders/jsons/shadowCubeMap.shaderImport","shadowCubeMapShader");
    auto skyboxModelId = assetManager.registerAsset("C:\\Users\\redkc\\CLionProjects\\ReasonableVulkan\\res\\models\\my\\Skybox\\Skybox.fbx","skyboxModel");
    auto planeId = assetManager.registerAsset("C:\\Users\\redkc\\CLionProjects\\ReasonableVulkan\\res\\models\\my\\Plane.fbx","planeModel");
    assetManager.registerAsset("C:/Users/redkc/CLionProjects/ReasonableVulkan/res/models/my/Box.fbx","boxModel");

    auto skyboxModelData = assetManager.getAssetData<am::ModelData>(skyboxModelId.value());
    auto skyboxMeshData = assetManager.getAssetData<am::MeshData>(skyboxModelData->rootNode.mChildren[0].meshes[0].get()->id);
    auto skyboxMaterialData = assetManager.getAssetData<am::MaterialData>(skyboxMeshData->material.get()->id);
    assetManager.getAssetData<am::TextureData>(skyboxMaterialData->diffuseTexture.get()->id)->type = am::TextureType::TextureCube;
    assetManager.saveAsset(skyboxMaterialData->diffuseTexture.get()->id);
    */

    vks::VulkanRenderer *vulkanRenderer = new vks::VulkanRenderer(&assetManager);
    engine::Engine engine = engine::Engine(platform,vulkanRenderer,&assetManager);
    engine.Initialize();


    vulkanRenderer->initialize(platform,1280, 720);

    /*
    // 3. Initialize the graphics abstraction
    gfx::Init();
    */
    // 4. Initialize game systems (ECS, scenes, etc.)
    /*
    auto scene = engine.CreateScene("Main scene");
    auto uuid = assetManager.createAsset(am::AssetType::Scene,"C:/Users/redkc/CLionProjects/ReasonableVulkan/res/models/my/scene","scene");
    scene.get()->sceneId = uuid.value();

    vulkanRenderer->loadModel(skyboxModelId.value());
    vulkanRenderer->loadModel(planeId.value());

    auto modelEntity = scene.get()->CreateEntity("Model");
    setLocalScale(scene.get()->GetComponent<TransformComponent>(modelEntity),{1,1,1});
    scene.get()->AddComponent<RendererComponent>(modelEntity,RendererComponent(planeId.value(), pbrShaderId.value()));
    scene.get()->GetComponent<TransformComponent>(modelEntity).position = glm::vec3(0,0,0);

    auto modelEntity2 = scene.get()->CreateEntity("Model");
    setLocalScale(scene.get()->GetComponent<TransformComponent>(modelEntity2),{1,-1,1});
    scene.get()->AddComponent<RendererComponent>(modelEntity2,RendererComponent(planeId.value(), pbrShaderId.value()));
    scene.get()->GetComponent<TransformComponent>(modelEntity2).position = glm::vec3(0,0,0);

    auto cameraEntity = scene.get()->CreateEntity("Camera");
    scene.get()->AddComponent<CameraComponent>(cameraEntity);

    scene.get()->GetComponent<CameraComponent>(cameraEntity).skyboxMaterialId = skyboxMeshData->material.get()->id;
    scene.get()->GetComponent<CameraComponent>(cameraEntity).active = true;

    auto spotLightEntity = scene.get()->CreateEntity("Spot Light");
    scene.get()->AddComponent<LightComponent>(spotLightEntity);
    auto& spotLight = scene.get()->GetComponent<LightComponent>(spotLightEntity);
    spotLight.hasShadow = true;
    spotLight.setType(LightComponent::Type::Spot);

    engine.SaveScene();

    */

    auto sceneId = assetManager.getAssetUuid("scene");
    engine.LoadScene(sceneId.value());

    // 5. Main loop
    bool running = true;
    while (running) {
        platform->PollEvents(running); // sets `running` to false on quit

        float deltaTime = platform->GetDeltaTime();
        engine.Update(deltaTime);     // game logic
    }

    // 6. Shutdown
    /*game::Shutdown();
    gfx::Shutdown();
    vulkan::Shutdown();*/
    platform->Shutdown();

    return EXIT_SUCCESS;
}
