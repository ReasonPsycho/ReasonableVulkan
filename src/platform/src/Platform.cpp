#include "Platform.hpp"
#include <imgui_impl_sdl3.h>

bool Platform::Init(const std::string& title, int width, int height) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow(
        title.c_str(),
        width, height,
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return false;
    }

    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    lastFrameTime = std::chrono::steady_clock::now();
    return true;
}


bool Platform::IsKeyPressed(int keyCode) const {
    auto it = keyStates.find(keyCode);
    return it != keyStates.end() && it->second;
}

void Platform::PollEvents(bool& running) {
    // Update last mouse position
    lastMouseX = mouseX;
    lastMouseY = mouseY;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);

        switch (event.type) {
            case SDL_EVENT_QUIT:
                running = false;
                break;

            case SDL_EVENT_WINDOW_RESIZED:
                if (event.window.windowID == SDL_GetWindowID(window)) {
                    WindowResizeEvent resizeEvent{event.window.data1, event.window.data2};
                    for (const auto& callback : eventCallbacks[EventType::WindowResize]) {
                        callback(&resizeEvent);
                    }
                }
                break;

        case SDL_EVENT_KEY_DOWN:
            keyStates[static_cast<int>(event.key.key)] = true;
            {
                KeyEvent keyEvent{
                    static_cast<int>(event.key.key),
                    static_cast<int>(event.key.scancode),
                    static_cast<unsigned>(event.key.mod),
                    event.key.repeat != 0
                };
                for (const auto& callback : eventCallbacks[EventType::KeyPressed]) {
                    callback(&keyEvent);
                }
            }
            break;

        case SDL_EVENT_KEY_UP:
            keyStates[static_cast<int>(event.key.key)] = false;
            {
                KeyEvent keyEvent{
                    static_cast<int>(event.key.key),
                    static_cast<int>(event.key.scancode),
                    static_cast<unsigned>(event.key.mod),
                    event.key.repeat != 0
                };
                for (const auto& callback : eventCallbacks[EventType::KeyReleased]) {
                    callback(&keyEvent);
                }
            }
            break;

            case SDL_EVENT_MOUSE_MOTION:
                mouseX = static_cast<float>(event.motion.x);
                mouseY = static_cast<float>(event.motion.y);
                {
                    MouseMoveEvent moveEvent{
                        mouseX, mouseY,
                        mouseX - lastMouseX,
                        mouseY - lastMouseY
                    };
                    for (const auto& callback : eventCallbacks[EventType::MouseMoved]) {
                        callback(&moveEvent);
                    }
                }
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                mouseButtonStates[event.button.button] = true;
                {
                    MouseButtonEvent buttonEvent{
                        event.button.button,
                        static_cast<float>(event.button.x),
                        static_cast<float>(event.button.y)
                    };
                    for (const auto& callback : eventCallbacks[EventType::MouseButtonPressed]) {
                        callback(&buttonEvent);
                    }
                }
                break;

            case SDL_EVENT_MOUSE_BUTTON_UP:
                mouseButtonStates[event.button.button] = false;
                {
                    MouseButtonEvent buttonEvent{
                        event.button.button,
                        static_cast<float>(event.button.x),
                        static_cast<float>(event.button.y)
                    };
                    for (const auto& callback : eventCallbacks[EventType::MouseButtonReleased]) {
                        callback(&buttonEvent);
                    }
                }
                break;

            case SDL_EVENT_MOUSE_WHEEL:
                {
                    MouseScrollEvent scrollEvent{
                        static_cast<float>(event.wheel.x),
                        static_cast<float>(event.wheel.y)
                    };
                    for (const auto& callback : eventCallbacks[EventType::MouseScrolled]) {
                        callback(&scrollEvent);
                    }
                }
                break;
        }
    }

    // Calculate delta time
    auto now = std::chrono::steady_clock::now();
    deltaTime = std::chrono::duration<float>(now - lastFrameTime).count();
    lastFrameTime = now;
}

void Platform::Shutdown() {
    eventCallbacks.clear();
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    SDL_Quit();
}

float Platform::GetDeltaTime() const {
    return deltaTime;
}

void Platform::SubscribeToEvent(EventType type, EventCallback callback) {
    eventCallbacks[type].push_back(callback);
}

void Platform::UnsubscribeFromEvent(EventType type, const EventCallback& callback) {
    auto& callbacks = eventCallbacks[type];
    callbacks.erase(
        std::remove_if(callbacks.begin(), callbacks.end(),
            [&callback](const EventCallback& cb) {
                return cb.target_type() == callback.target_type();
            }
        ),
        callbacks.end()
    );
}

void Platform::GetWindowSize(int& width, int& height) const {
    SDL_GetWindowSize(window, &width, &height);
}

bool Platform::IsWindowMinimized() const {
    return (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) != 0;
}

bool Platform::IsWindowFocused() const {
    return (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0;
}


bool Platform::IsMouseButtonPressed(uint8_t button) const {
    auto it = mouseButtonStates.find(button);
    return it != mouseButtonStates.end() && it->second;
}

void Platform::GetMousePosition(float& x, float& y) const {
    x = mouseX;
    y = mouseY;
}