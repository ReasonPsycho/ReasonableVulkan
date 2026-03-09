#define SDL_MAIN_HANDLED

#include <iostream>
#include <cstdlib>
#include <glm/ext/matrix_transform.hpp>
#include <SDL3/SDL.h>
#include "platform/src/Platform.hpp"
#include "platform/include/PlatformInterface.hpp"
#include "assetManager/src/AssetManager.hpp"
#include "ecs/Scene.h"
#include "engine/Engine.h"
#include "systems/renderingSystem/componets/CameraComponent.hpp"
#include "systems/renderingSystem/componets/LightComponent.hpp"
#include "systems/renderingSystem/componets/RendererComponent.hpp"
#include "vks/VulkanRenderer.h"



int main(int argc, char *argv[]) {
    plt::PlatformInterface* platform = new plt::Platform();
    // 1. Initialize platform (SDL window, input, etc.)
    if (!platform->Init("My Game Engine", 1280, 720)) {
        return EXIT_FAILURE;
    }

    am::AssetManagerInterface& assetManager = am::AssetManager::getInstance();
    vks::VulkanRenderer *vulkanExample = new vks::VulkanRenderer(&assetManager);
    engine::Engine engine = engine::Engine(platform,vulkanExample,&assetManager);
    engine.Initialize();


    vulkanExample->initialize(platform,1280, 720);

    /*
    // 3. Initialize the graphics abstraction
    gfx::Init();
    */
    // 4. Initialize game systems (ECS, scenes, etc.)
    auto scene = engine.CreateScene("Main scene");

    auto shader = assetManager.registerAsset("C:\\Users\\redkc\\CLionProjects\\ReasonableVulkan\\res\\shaders\\jsons\\pbr.shader");
    auto shaderData = assetManager.getAsset(shader->get()->id);

    auto asset = assetManager.registerAsset("C:/Users/redkc/CLionProjects/ReasonableVulkan/res/models/my/Plane.fbx");
    vulkanExample->loadModel(asset->get()->id);

    auto modelEntity = scene.get()->CreateEntity("Model");
    setLocalScale(scene.get()->GetComponent<Transform>(modelEntity),{1,1,1});
    scene.get()->AddComponent<RendererComponent>(modelEntity,RendererComponent(asset->get()->id));
    scene.get()->GetComponent<Transform>(modelEntity).position = glm::vec3(0,0,0);

    auto cameraEntity = scene.get()->CreateEntity("Camera");
    scene.get()->AddComponent<CameraComponent>(cameraEntity);

    auto lightEntity = scene.get()->CreateEntity("Light");
    scene.get()->AddComponent<LightComponent>(lightEntity);

    scene.get()->CreateEntity(); //Empty


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