#define SDL_MAIN_HANDLED

#include <iostream>
#include <cstdlib>
#include <SDL2/SDL.h>
#include "platform.hpp"
#include "vks/vulkanRenderer.h"

int main(int argc, char *argv[]) {
    VulkanExampleBase *vulkanExample = new VulkanExampleBase();

    // 1. Initialize platform (SDL window, input, etc.)
    if (!platform::Init("My Game Engine", 1280, 720)) {
        return EXIT_FAILURE;
    }

    if (!vulkanExample->initVulkan()) {
        return EXIT_FAILURE;
    }

    platform::WindowInfo window_info = platform::GetWindowInfo();
    vulkanExample->setupWindow(window_info.hInstance, window_info.wndProc, window_info.hwnd);
    vulkanExample->prepare();


    /*
    // 3. Initialize the graphics abstraction
    gfx::Init();

    // 4. Initialize game systems (ECS, scenes, etc.)
    game::Init();*/

    // 5. Main loop
    bool running = true;
    while (running) {
        platform::PollEvents(running); // sets `running` to false on quit

        float deltaTime = platform::GetDeltaTime();
        vulkanExample->render();
        /*game::Update(deltaTime);     // game logic
        gfx::BeginFrame();           // prepare frame
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
