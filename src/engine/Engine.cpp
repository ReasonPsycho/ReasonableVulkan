#include "Engine.h"

#include "ecs/Scene.h"

namespace engine {
    void Engine::Initialize()
    {
        platform->SubscribeToEvent(PlatformInterface::EventType::WindowMinimize,
      [this](const void* /*data*/) {
          minimized = true;
      });

        platform->SubscribeToEvent(PlatformInterface::EventType::WindowRestored,
            [this](const void* /*data*/) {
                minimized = false;
            });
    }

    std::shared_ptr<Scene> Engine::CreateScene(const std::string& name) {
        if (scenes.find(name) != scenes.end()) {
            return scenes[name]; // Scene already exists, return it
        }

        auto scene = std::make_shared<Scene>(*this);  // Changed from (this) to (*this)
        scenes[name] = scene;

        // Optionally set it active if it's the first one
        if (!activeScene) {
            activeScene = scene;
        }

        return scene;
    }

    std::shared_ptr<Scene> Engine::GetScene(const std::string& name) {
        auto it = scenes.find(name);
        if (it != scenes.end()) {
            return it->second;
        }
        return nullptr;
    }

    void Engine::RemoveScene(const std::string& name) {
        auto it = scenes.find(name);
        if (it != scenes.end()) {
            if (activeScene == it->second) {
                activeScene = nullptr;
            }
            scenes.erase(it);
        }
    }

    void Engine::SetActiveScene(const std::string& name) {
        auto it = scenes.find(name);
        if (it != scenes.end()) {
            activeScene = it->second;
        }
    }

    std::shared_ptr<Scene> Engine::GetActiveScene() {
        return activeScene;
    }

    void Engine::Update(float deltaTime) {
        if (activeScene) {
            activeScene->Update(deltaTime);
        }
    }
} // namespace engine