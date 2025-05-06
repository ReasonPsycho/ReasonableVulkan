//
// Created by redkc on 04.05.2025.
//

#ifndef PLATFORM_HPP
#define PLATFORM_HPP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <SDL2/SDL.h>
#include <string>
#include <chrono>
#include <libloaderapi.h>
#include <winuser.h>
#include <SDL2/SDL_syswm.h>


namespace platform {
    struct WindowInfo {
        HMODULE hInstance;
        WNDPROC wndProc;
        HWND hwnd;
    };

    bool Init(const std::string& title, int width, int height);
    void PollEvents(bool& running);
    void Shutdown();

    SDL_Window* GetWindow();

    WindowInfo GetWindowInfo();

    float GetDeltaTime();
}

#endif //PLATFORM_HPP
