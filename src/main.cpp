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
#include "vks/VulkanRenderer.h"



int main(int argc, char *argv[]) {
    PlatformInterface* platform = new Platform();
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

    auto asset = assetManager.registerAsset("C:/Users/redkc/CLionProjects/ReasonableVulkan/res/models/my/Plane.fbx");
    vulkanExample->loadModel(asset->get()->id);

    auto modelEntity = scene.get()->CreateEntity("Model");
    setLocalScale(scene.get()->GetComponent<Transform>(modelEntity),{1,1,1});
    scene.get()->AddComponent<Model>(modelEntity,Model(asset->get()->id));

    auto cameraEntity = scene.get()->CreateEntity("Camera");
    scene.get()->AddComponent<Camera>(cameraEntity,Camera());

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