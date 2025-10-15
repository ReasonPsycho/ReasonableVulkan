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
#include "PlatformInterface.hpp"

#include "ecs/componentArrays/ComponentArray.h"
#include "ecs/componentArrays/IntegralComponentArray.h"

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
        Engine(PlatformInterface* platformInterface,gfx::GraphicsEngine* graphicsEngine, am::AssetManagerInterface* assetManagerInterface);
        ;
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
        PlatformInterface* platform;
        bool minimized = false;

        template<typename T>
        void RegisterComponentType() {
            auto type = std::type_index(typeid(T));
            componentTypes.insert(type);
            componentFactories[type] = []() -> std::shared_ptr<IComponentArray> {
                if constexpr (std::is_same_v<T, Transform>) {
                    return std::make_shared<IntegralComponentArray<T>>();
                } else {
                    return std::make_shared<ComponentArray<T>>();
                }
            };
        }

        // System factory registration
        template<typename T>
        void RegisterSystemType() {
            auto type = std::type_index(typeid(T));
            systemTypes.insert(type);
            systemFactories[type] = [](Scene* scene) -> std::shared_ptr<SystemBase> {
                return std::make_shared<T>(scene);
            };
        }

        // Factory getters
        std::shared_ptr<IComponentArray> CreateComponentArray(const std::type_index& type) const {
            auto it = componentFactories.find(type);
            if (it != componentFactories.end()) {
                return it->second();
            }
            throw std::runtime_error("No factory registered for component type: " + std::string(type.name()));
        }

        std::shared_ptr<SystemBase> CreateSystem(const std::type_index& type, Scene* scene) const {
            auto it = systemFactories.find(type);
            if (it != systemFactories.end()) {
                return it->second(scene);
            }
            throw std::runtime_error("No factory registered for system type: " + std::string(type.name()));
        }

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

} // namespace engine


#endif //ENGINE_H
