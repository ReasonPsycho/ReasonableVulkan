//
// Created by redkc on 29/07/2025.
//

#ifndef ENGINE_H
#define ENGINE_H
#include <memory>
#include "ecs/Scene.h"

namespace engine {

    using namespace engine::ecs;

    class Engine {
    public:
        static Engine& GetInstance();

        // Scene management
        std::shared_ptr<Scene> CreateScene(const std::string& name);
        std::shared_ptr<Scene> GetScene(const std::string& name);
        void RemoveScene(const std::string& name);
        void SetActiveScene(const std::string& name);
        std::shared_ptr<Scene> GetActiveScene();

        // Global update loop
        void Update(float deltaTime);

    private:
        Engine() = default;
        ~Engine() = default;

        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;

        std::unordered_map<std::string, std::shared_ptr<Scene>> scenes;
        std::shared_ptr<Scene> activeScene = nullptr;
    };

} // namespace engine


#endif //ENGINE_H
