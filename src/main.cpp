#define SDL_MAIN_HANDLED

#include <iostream>
#include <cstdlib>
#include <SDL2/SDL.h>
#include "platform.hpp"
#include "assetManager/src/AssetManager.hpp"
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

    // Create model matrix - you might want to adjust these values based on your model
    glm::mat4 modelMatrix = glm::mat4(1.0f);  // Identity matrix as starting point
    modelMatrix = glm::rotate(modelMatrix, glm::radians(-45.0f), glm::vec3(0.5f, 0.5f, 0.0f)); // Rotate model upright
    modelMatrix = glm::scale(modelMatrix, glm::vec3(5.0f));  // Scale if needed


    // 5. Main loop
    bool running = true;
    while (running) {
        platform::PollEvents(running); // sets `running` to false on quit

        float deltaTime = platform::GetDeltaTime();
        vulkanExample->drawModel(asset->get()->id,modelMatrix);
        vulkanExample->beginFrame();
        vulkanExample->renderFrame();
        vulkanExample->endFrame();
        engine.Update(deltaTime);     // game logic
    }

    // 6. Shutdown
    /*game::Shutdown();
    gfx::Shutdown();
    vulkan::Shutdown();*/
    platform::Shutdown();

    return EXIT_SUCCESS;
}