#include "Platform.hpp"
#include <imgui_impl_sdl3.h>

namespace  plt
{
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
        // Process queued events from other threads
        {
            std::lock_guard<std::mutex> lock(queuedEventsMutex);
            for (const auto& qEvent : queuedEvents) {
                for (const auto& callback : eventCallbacks[qEvent.type]) {
                    callback(qEvent.data.get());
                }
            }
            queuedEvents.clear();
        }

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

            case SDL_EVENT_WINDOW_MINIMIZED:
                if (event.window.windowID == SDL_GetWindowID(window)) {
                    for (const auto& callback : eventCallbacks[EventType::WindowMinimize]) {
                        callback(nullptr);
                    }
                }
                break;

            case SDL_EVENT_WINDOW_MAXIMIZED:
                if (event.window.windowID == SDL_GetWindowID(window)) {
                    for (const auto& callback : eventCallbacks[EventType::WindowMaximize]) {
                        callback(nullptr);
                    }
                }
                break;

            case SDL_EVENT_WINDOW_RESTORED:
                if (event.window.windowID == SDL_GetWindowID(window)) {
                    for (const auto& callback : eventCallbacks[EventType::WindowRestored]) {
                        callback(nullptr);
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
            case SDL_EVENT_DROP_FILE:
                if (event.drop.windowID == SDL_GetWindowID(window)) {
                    FileDropEvent dropEvent{event.drop.data};
                    for (const auto& callback : eventCallbacks[EventType::FileDropped]) {
                        callback(&dropEvent);
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
        {
            std::lock_guard<std::mutex> lock(watchersMutex);
            for (auto& pair : folderWatchers) {
                pair.second->stopWatching = true;
#ifdef _WIN32
                CancelIoEx(pair.second->directoryHandle, nullptr);
                if (pair.second->watchThread.joinable()) {
                    pair.second->watchThread.join();
                }
                CloseHandle(pair.second->directoryHandle);
#else
                // Non-windows shutdown logic (e.g. close inotify fd)
                if (pair.second->watchThread.joinable()) {
                    pair.second->watchThread.join();
                }
#endif
            }
            folderWatchers.clear();
        }
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

    void Platform::GetWindowPosition(int& x, int& y) const {
        SDL_GetWindowPosition(window, &x, &y);
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

    void Platform::WatchFolder(const std::string& path) {
        std::lock_guard<std::mutex> lock(watchersMutex);
        if (folderWatchers.find(path) != folderWatchers.end()) return;

#ifdef _WIN32
        HANDLE hDir = CreateFileA(
            path.c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            NULL
        );

        if (hDir == INVALID_HANDLE_VALUE) {
            return;
        }

        auto info = std::make_shared<WatcherInfo>();
        info->path = path;
        info->directoryHandle = hDir;
        info->stopWatching = false;
        info->watchThread = std::thread(&Platform::WatcherThread, this, info);

        folderWatchers[path] = info;
#else
        // Non-windows implementation placeholder
        // e.g. using inotify_add_watch on Linux
#endif
    }

    void Platform::UnwatchFolder(const std::string& path) {
        std::lock_guard<std::mutex> lock(watchersMutex);
        auto it = folderWatchers.find(path);
        if (it != folderWatchers.end()) {
            it->second->stopWatching = true;
#ifdef _WIN32
            CancelIoEx(it->second->directoryHandle, nullptr);
            if (it->second->watchThread.joinable()) {
                it->second->watchThread.join();
            }
            CloseHandle(it->second->directoryHandle);
#else
            if (it->second->watchThread.joinable()) {
                it->second->watchThread.join();
            }
#endif
            folderWatchers.erase(it);
        }
    }

    void Platform::WatcherThread(std::shared_ptr<WatcherInfo> info) {
#ifdef _WIN32
        char buffer[1024];
        DWORD bytesReturned;

        while (!info->stopWatching) {
            if (ReadDirectoryChangesW(
                info->directoryHandle,
                buffer,
                sizeof(buffer),
                TRUE,
                FILE_NOTIFY_CHANGE_FILE_NAME,
                &bytesReturned,
                NULL,
                NULL
            )) {
                if (info->stopWatching) break;

                FILE_NOTIFY_INFORMATION* fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer);
                do {
                    if (fni->Action == FILE_ACTION_ADDED) {
                        int fileNameLength = fni->FileNameLength / sizeof(WCHAR);
                        std::wstring wFileName(fni->FileName, fileNameLength);
                        std::string fileName(wFileName.begin(), wFileName.end());

                        auto eventData = std::make_shared<FileAddedEvent>();
                        eventData->filePath = info->path + "/" + fileName;
                        eventData->folderPath = info->path;

                        {
                            std::lock_guard<std::mutex> lock(queuedEventsMutex);
                            queuedEvents.push_back({EventType::FileAddedToFolder, eventData});
                        }
                    }
                    if (fni->NextEntryOffset == 0) break;
                    fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                        reinterpret_cast<BYTE*>(fni) + fni->NextEntryOffset
                    );
                } while (true);
            } else {
                if (GetLastError() == ERROR_OPERATION_ABORTED) break;
                // Some other error, maybe wait a bit
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
#else
        // Non-windows implementation placeholder
#endif
    }
}