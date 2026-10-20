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
#include "EngineInterface.hpp"
#include "events/EventDispatcher.hpp"

namespace engine {
    namespace ecs
    {
        class SystemBase;
        struct TransformComponent;
        class Scene;
        class EditorSystem;
    }

    using namespace engine::ecs;

    class Engine : public EngineInterface{
    public:
        Engine(plt::PlatformInterface* platformInterface, gfx::GraphicsEngine* graphicsEngine, am::AssetManagerInterface* assetManagerInterface);
        ~Engine() override;

        void Initialize() override;

        // Scene management
        std::shared_ptr<Scene> CreateScene(const std::string& name) override;
        std::shared_ptr<Scene> GetScene(const std::string& name) override;
        void RemoveScene(const std::string& name) override;
        void SetActiveScene(const std::string& name, bool active = true) override;
        void SetSceneActive(const std::string& name, bool active) override;
        std::shared_ptr<Scene> GetActiveScene() override;
        std::vector<std::shared_ptr<Scene>> GetActiveScenes() override;
        const std::unordered_map<std::string, std::shared_ptr<Scene>>& GetScenes() const override { return scenes; }
        std::unordered_map<std::string, std::shared_ptr<Scene>>& GetScenes() override { return scenes; }
        const std::vector<std::string>& GetSceneOrder() const override { return sceneOrder; }
        std::shared_ptr<Scene> GetTopEditableScene() override;

        std::shared_ptr<editor::EditorInterface> GetEditor() override;
        std::shared_ptr<ecs::EditorSystem> GetEditorSystem();

        // Global update loop
        void Update(float deltaTime) override;

        // Play/Pause mode
        bool IsPlaying() const override { return isPlaying; }
        bool IsPaused() const override { return isPaused; }
        void StartPlayMode() override;
        void StopPlayMode() override;
        void SetPaused(bool paused) override;
        void TogglePlay() override;
        void TogglePause() override;

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

        void SaveScene() override;
        void LoadScene(boost::uuids::uuid sceneId) override;
        void CloseScene(const std::string& name) override;
        void CloseScene(boost::uuids::uuid sceneId) override;

        void SaveConfig() override;
        void LoadConfig() override;
        std::string configLookupName = "engineConfig";

        bool SetupAssetInScene(ecs::Scene* scene, const boost::uuids::uuid& assetId);

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

        // Events
        events::EventDispatcher& GetEventDispatcher() { return eventDispatcher; }
        const events::EventDispatcher& GetEventDispatcher() const { return eventDispatcher; }

        template<typename TEvent>
        void PublishEvent(const TEvent& event) {
            eventDispatcher.Publish(event);
        }

        template<typename TEvent>
        void EnqueueEvent(const TEvent& event) {
            eventDispatcher.Enqueue(event);
        }

        void ProcessEvents() {
            eventDispatcher.ProcessAllQueues();
        }

    private:
        events::EventDispatcher eventDispatcher;
        std::unordered_map<std::string, std::shared_ptr<Scene>> scenes;
        std::vector<std::string> sceneOrder;
        std::shared_ptr<ecs::EditorSystem> editorSystem = nullptr;

        bool isPlaying = false;
        bool isPaused = false;
        std::unordered_map<std::string, rapidjson::Document> sceneSnapshots;
    };

} // namespace engine


#endif //ENGINE_H
