
#ifndef PLATFORM_HPP
#define PLATFORM_HPP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <SDL3/SDL.h>
#include <unordered_map>
#include <vector>
#include "PlatformInterface.hpp"

namespace  plt
{


class Platform : public PlatformInterface {
public:
    Platform() : PlatformInterface()
    {

    }
    // Main platform functions
    bool Init(const std::string& title, int width, int height) override;
    void PollEvents(bool& running) override;
    void Shutdown() override;
    void* GetNativeWindow() const override { return window; }
    float GetDeltaTime() const override;

    // Event system functions
    void SubscribeToEvent(EventType type, EventCallback callback) override;
    void UnsubscribeFromEvent(EventType type, const EventCallback& callback) override;

    // Window utility functions
    void GetWindowSize(int& width, int& height) const override;
    void GetWindowPosition(int& x, int& y) const override;
    bool IsWindowMinimized() const override;
    bool IsWindowFocused() const override;

    // Input state functions
    bool IsKeyPressed(int keyCode) const override;
    bool IsMouseButtonPressed(uint8_t button) const override;
    void GetMousePosition(float& x, float& y) const override;

private:
    SDL_Window* window = nullptr;
    std::chrono::steady_clock::time_point lastFrameTime;
    float deltaTime = 0.0f;

    // Event system storage
    std::unordered_map<EventType, std::vector<EventCallback>> eventCallbacks;

    // Input state storage
    std::unordered_map<int, bool> keyStates;
    std::unordered_map<uint8_t, bool> mouseButtonStates;
    float mouseX = 0.0f, mouseY = 0.0f;
    float lastMouseX = 0.0f, lastMouseY = 0.0f;
};


}
#endif //PLATFORM_HPP