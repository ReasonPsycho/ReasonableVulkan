//
// Created by redkc on 29/07/2025.
//

#ifndef ENGINE_H
#define ENGINE_H
#include <fstream>
#include <memory>
#include <set>
#include <unordered_map>
#include <rapidjson/prettywriter.h>
#include <spdlog/spdlog.h>

#include "AssetManagerInterface.h"
#include "GraphicsEngine.hpp"
#include "ecs/componentArrays/IntegralComponentArray.h"
#include "ecs/componentArrays/ComponentArray.h"

namespace engine {
    namespace ecs
    {
        class SystemBase;
        struct Transform;
        class Scene;
    }

    using namespace engine::ecs;

    class Engine {
    public:
        Engine(plt::PlatformInterface* platformInterface,gfx::GraphicsEngine* graphicsEngine, am::AssetManagerInterface* assetManagerInterface);
        ~Engine() = default;

        void Initialize();

        // Scene management
        std::shared_ptr<Scene> CreateScene(const std::string& name);
        std::shared_ptr<Scene> GetScene(const std::string& name);
        void RemoveScene(const std::string& name);
        void SetActiveScene(const std::string& name);
        std::shared_ptr<Scene> GetActiveScene();


        // Global update loop
        void Update(float deltaTime);

        am::AssetManagerInterface* assetManagerInterface;
        gfx::GraphicsEngine* graphicsEngine;
        plt::PlatformInterface* platform;
        bool minimized = false;

        template<typename T>
        void RegisterComponentType();

        template <class T>
        void RegisterSystemType();


        // Factory getters
        std::shared_ptr<IComponentArray> CreateComponentArray(const std::type_index& type) const;
        std::shared_ptr<SystemBase> CreateSystem(const std::type_index& type, Scene* scene) const;

        void SaveScene(std::string filename);
        void LoadScene(std::string filename);

        // Get registered types
        const std::set<std::type_index>& GetRegisteredComponentTypes() const { return componentTypes; }
        const std::set<std::type_index>& GetRegisteredSystemTypes() const { return systemTypes; }

        std::unordered_map<std::type_index, std::function<std::shared_ptr<IComponentArray>()>> componentFactories;
        std::unordered_map<std::type_index, std::function<std::shared_ptr<SystemBase>(Scene*)>> systemFactories;

    private:
        std::set<std::type_index> componentTypes;
        std::set<std::type_index> systemTypes;

        std::unordered_map<std::string, std::shared_ptr<Scene>> scenes;
        std::shared_ptr<Scene> activeScene = nullptr;
    };


#include "Engine.tpp"

} // namespace engine


#endif //ENGINE_H
