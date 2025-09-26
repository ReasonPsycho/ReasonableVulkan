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
    engine::Engine& engine = engine::Engine::GetInstance();

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

    // 5. Main loop
    bool running = true;
    while (running) {
        platform::PollEvents(running); // sets `running` to false on quit

        float deltaTime = platform::GetDeltaTime();
        vulkanExample->render();
        engine.Update(deltaTime);     // game logic
        /*gfx::BeginFrame();           // prepare frame
        game::Render();              // game rendering logic (calls gfx underneath)
        gfx::EndFrame();             // submit frame*/
    }

    // 6. Shutdown
    /*game::Shutdown();
    gfx::Shutdown();
    vulkan::Shutdown();*/
    platform::Shutdown();

    return EXIT_SUCCESS;
}