
#ifndef REASONABLEVULKAN_PLATFORMINTERFACE_HPP
#define REASONABLEVULKAN_PLATFORMINTERFACE_HPP

#include <string>
#include <functional>
#include <chrono>

class PlatformInterface {
public:
    // Event system types
    enum class EventType {
        WindowResize,
        WindowMinimize,
        WindowMaximize,
        WindowFocus,
        WindowLostFocus,
        KeyPressed,
        KeyReleased,
        MouseMoved,
        MouseButtonPressed,
        MouseButtonReleased,
        MouseScrolled
    };

    // Event data structures
    struct WindowResizeEvent {
        int width;
        int height;
    };

    struct KeyEvent {
        int keyCode;        // Virtual key code
        int scancode;       // Physical key code
        unsigned modifiers; // Key modifiers (shift, ctrl, etc.)
        bool isRepeat;      // Whether this is a key repeat
    };

    struct MouseMoveEvent {
        float x;
        float y;
        float deltaX;
        float deltaY;
    };

    struct MouseButtonEvent {
        uint8_t button;
        float x;
        float y;
    };

    struct MouseScrollEvent {
        float xOffset;
        float yOffset;
    };

    using EventCallback = std::function<void(const void*)>;

    virtual ~PlatformInterface() = default;

    // Main platform functions
    virtual bool Init(const std::string& title, int width, int height) = 0;
    virtual void PollEvents(bool& running) = 0;
    virtual void Shutdown() = 0;
    virtual void* GetNativeWindow() const = 0;
    virtual float GetDeltaTime() const = 0;

    // Event system functions
    virtual void SubscribeToEvent(EventType type, EventCallback callback) = 0;
    virtual void UnsubscribeFromEvent(EventType type, const EventCallback& callback) = 0;

    // Window utility functions
    virtual void GetWindowSize(int& width, int& height) const = 0;
    virtual bool IsWindowMinimized() const = 0;
    virtual bool IsWindowFocused() const = 0;

    // Input state functions
    virtual bool IsKeyPressed(int keyCode) const = 0;
    virtual bool IsMouseButtonPressed(uint8_t button) const = 0;
    virtual void GetMousePosition(float& x, float& y) const = 0;

protected:
    PlatformInterface() = default;
};

#endif //REASONABLEVULKAN_PLATFORMINTERFACE_HPP