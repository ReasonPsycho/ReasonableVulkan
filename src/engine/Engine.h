//
// Created by redkc on 29/07/2025.
//

#ifndef ENGINE_H
#define ENGINE_H
#include <memory>
#include <unordered_map>

#include "AssetManagerInterface.h"
#include "GraphicsEngine.hpp"

namespace engine {
    namespace ecs
    {
        class Scene;
    }

    using namespace engine::ecs;

    class Engine {
    public:
        Engine(gfx::GraphicsEngine* graphicsEngine, am::AssetManagerInterface* assetManagerInterface) : graphicsEngine(graphicsEngine), assetManagerInterface(assetManagerInterface){};
        ~Engine() = default;

        // Scene management
        std::shared_ptr<Scene> CreateScene(const std::string& name);
        std::shared_ptr<Scene> GetScene(const std::string& name);
        void RemoveScene(const std::string& name);
        void SetActiveScene(const std::string& name);
        std::shared_ptr<Scene> GetActiveScene();

        // Global update loop
        void Update(float deltaTime);


        gfx::GraphicsEngine* graphicsEngine;

    private:

        std::unordered_map<std::string, std::shared_ptr<Scene>> scenes;
        std::shared_ptr<Scene> activeScene = nullptr;
        am::AssetManagerInterface* assetManagerInterface;
    };

} // namespace engine


#endif //ENGINE_H
