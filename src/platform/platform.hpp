//
// Created by redkc on 04.05.2025.
//

#ifndef PLATFORM_HPP
#define PLATFORM_HPP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <SDL3/SDL.h>
#include <string>
#include <chrono>
#include <libloaderapi.h>
#include <winuser.h>


namespace platform {
    bool Init(const std::string& title, int width, int height);
    void PollEvents(bool& running);
    void Shutdown();

    SDL_Window* GetWindow();

    float GetDeltaTime();
}

#endif //PLATFORM_HPP
