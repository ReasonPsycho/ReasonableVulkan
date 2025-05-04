//
// Created by redkc on 04.05.2025.
//

#ifndef PLATFORM_HPP
#define PLATFORM_HPP

#include <SDL2/SDL.h>
#include <string>
#include <chrono>

namespace platform {
    bool Init(const std::string& title, int width, int height);
    void PollEvents(bool& running);
    void Shutdown();

    SDL_Window* GetWindow();
    float GetDeltaTime();
}

#endif //PLATFORM_HPP
