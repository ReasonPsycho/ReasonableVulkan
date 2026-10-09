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

        std::shared_ptr<ecs::EditorSystem> GetEditorSystem() override;


        // Global update loop
        void Update(float deltaTime) override;

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
        bool SaveEntityAsPrefab(uint32_t entity, const std::filesystem::path& destinationDirectoryOrPath, ecs::Scene* sourceScene = nullptr) override;

        void SaveConfig() override;
        void LoadConfig() override;
        std::string configLookupName = "engineConfig";

        void* GetThumbnailTexture(const boost::uuids::uuid& assetId, const std::string& thumbnailPath) override;
        bool CaptureModelThumbnail(const boost::uuids::uuid& modelId, const std::string& outputPath) override;
        bool CaptureMaterialThumbnail(const boost::uuids::uuid& materialId, const std::string& outputPath) override;
        std::shared_ptr<ecs::Scene> OpenModelPreviewScene(const boost::uuids::uuid& modelOrMeshId) override;

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
        std::vector<std::string> sceneOrder;
        std::shared_ptr<ecs::EditorSystem> editorSystem = nullptr;
    };

} // namespace engine


#endif //ENGINE_H
