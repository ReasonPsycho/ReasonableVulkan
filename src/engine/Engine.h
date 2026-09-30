//
// Created by redkc on 29/07/2025.
//

#ifndef ENGINE_H
#define ENGINE_H
#include <fstream>
#include <memory>
#include <set>
#include <unordered_map>
#include <optional>
#include <string_view>
#include <rapidjson/prettywriter.h>
#include <spdlog/spdlog.h>

#include "AssetManagerInterface.h"
#include "GraphicsEngine.hpp"
#include "ecs/componentArrays/ComponentType.h"
#include "ecs/componentArrays/IntegralComponentArray.h"
#include "ecs/componentArrays/ComponentArray.h"
#include "ecs/SystemBase.h"

namespace engine {
    namespace ecs
    {
        class SystemBase;
        struct TransformComponent;
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
        void RegisterComponentType() {}

        template <class T>
        void RegisterSystemType() {}


        // Factory getters
        std::shared_ptr<IComponentArray> CreateComponentArray(const std::type_index& type) const;
        std::shared_ptr<SystemBase> CreateSystem(const std::type_index& type, Scene* scene) const;

        void SaveScene();
        void LoadScene(boost::uuids::uuid sceneId);

        // Get registered types
        const std::set<std::type_index>& GetRegisteredComponentTypes() const;
        const std::set<std::type_index>& GetRegisteredSystemTypes() const;

        // Name-based type lookup
        std::optional<std::type_index> GetComponentTypeByName(std::string_view name) const;
        std::optional<std::type_index> GetSystemTypeByName(std::string_view name) const;

        std::string_view GetComponentTypeName(const std::type_index& type) const;
        std::string_view GetSystemTypeName(const std::type_index& type) const;

        ComponentTypeID GetComponentTypeID(const std::type_index& type) const;
        std::type_index GetComponentTypeFromID(ComponentTypeID id) const;

    private:
        std::unordered_map<std::string, std::shared_ptr<Scene>> scenes;
        std::shared_ptr<Scene> activeScene = nullptr;
    };

} // namespace engine


#endif //ENGINE_H
