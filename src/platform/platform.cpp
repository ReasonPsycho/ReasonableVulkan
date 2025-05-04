//
// Created by redkc on 04.05.2025.
//

#include "platform.hpp"

namespace platform {
    static SDL_Window* window = nullptr;
    static SDL_Event event;

    static std::chrono::steady_clock::time_point lastFrameTime;
    static float deltaTime = 0.0f;

    bool Init(const std::string& title, int width, int height) {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
            SDL_Log("SDL_Init failed: %s", SDL_GetError());
            return false;
        }

        window = SDL_CreateWindow(
            title.c_str(),
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            width, height,
            SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
        );

        if (!window) {
            SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
            return false;
        }

        lastFrameTime = std::chrono::steady_clock::now();
        return true;
    }

    void PollEvents(bool& running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            // Add keyboard/mouse input here if needed
        }

        // Calculate delta time
        auto now = std::chrono::steady_clock::now();
        deltaTime = std::chrono::duration<float>(now - lastFrameTime).count();
        lastFrameTime = now;
    }

    void Shutdown() {
        if (window) {
            SDL_DestroyWindow(window);
            window = nullptr;
        }
        SDL_Quit();
    }

    SDL_Window* GetWindow() {
        return window;
    }

    float GetDeltaTime() {
        return deltaTime;
    }
}