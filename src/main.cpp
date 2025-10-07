#define SDL_MAIN_HANDLED

#include <iostream>
#include <cstdlib>
#include <glm/ext/matrix_transform.hpp>
#include <SDL2/SDL.h>
#include "platform.hpp"
#include "assetManager/src/AssetManager.hpp"
#include "ecs/Scene.h"
#include "engine/Engine.h"
#include "vks/VulkanRenderer.h"



int main(int argc, char *argv[]) {
    am::AssetManagerInterface& assetManager = am::AssetManager::getInstance();
    vks::VulkanRenderer *vulkanExample = new vks::VulkanRenderer(&assetManager);
    engine::Engine engine = engine::Engine(vulkanExample,&assetManager);

    // 1. Initialize platform (SDL window, input, etc.)
    if (!platform::Init("My Game Engine", 1280, 720)) {
        return EXIT_FAILURE;
    }

    platform::WindowInfo window_info = platform::GetWindowInfo();
    vulkanExample->initialize(window_info.hwnd,1280, 720);

    /*
    // 3. Initialize the graphics abstraction
    gfx::Init();
    */
    // 4. Initialize game systems (ECS, scenes, etc.)
    auto scene = engine.CreateScene("Main scene");


    auto asset = assetManager.registerAsset("C:/Users/redkc/CLionProjects/ReasonableVulkan/res/models/my/Plane.fbx");
    vulkanExample->loadModel(asset->get()->id);

    auto modelEntity = scene.get()->CreateEntity();
    setLocalScale(scene.get()->GetComponent<Transform>(modelEntity),{5,5,5});
    scene.get()->AddComponent<Model>(modelEntity,{asset->get()->id});


    float aspectRatio = 1280 / (float)720; // Use your window's width and height
    glm::mat4 projectionMatrix = glm::perspective(
        glm::radians(45.0f),  // 45 degree field of view
        aspectRatio,
        0.1f,                 // Near plane
        100.0f               // Far plane
    );

    // Position camera slightly back and up
    glm::vec3 cameraPos = glm::vec3(0.0f, 2.0f, -12.0f);
    glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);  // Look at center
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 viewMatrix = glm::lookAt(cameraPos, cameraTarget, cameraUp);

    // Set light position above and slightly in front of the model
    glm::vec4 lightPosition = glm::vec4(0.0f, 5.0f, 2.0f,1.0f);

    auto cameraEntity = scene.get()->CreateEntity();
    scene.get()->AddComponent<Camera>(cameraEntity,{projectionMatrix,viewMatrix,lightPosition});

    // 5. Main loop
    bool running = true;
    while (running) {
        platform::PollEvents(running); // sets `running` to false on quit

        float deltaTime = platform::GetDeltaTime();
        engine.Update(deltaTime);     // game logic
    }

    // 6. Shutdown
    /*game::Shutdown();
    gfx::Shutdown();
    vulkan::Shutdown();*/
    platform::Shutdown();

    return EXIT_SUCCESS;
}