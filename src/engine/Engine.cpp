#include "Engine.h"

#include "PlatformInterface.hpp"
#include "../assetManager/src/assets/engineAssets/SceneAsset.h"
#include "ecs/Scene.h"
#include "systems/collisionSystem/CollisionSystem.hpp"
#include "systems/editorSystem/EditorSystem.hpp"
#include "systems/gizmoSystem/GizmoSystem.hpp"
#include "systems/renderingSystem/RenderSystem.h"
#include "systems/transformSystem/TransformSystem.h"
#include "systems/renderingSystem/componets/MeshComponent.hpp"
#include "systems/renderingSystem/componets/RendererComponent.hpp"
#include "systems/renderingSystem/componets/CameraComponent.hpp"
#include "systems/renderingSystem/componets/LightComponent.hpp"
#include "systems/transformSystem/componets/TransformComponent.hpp"
#include "ecs/NameComponent.hpp"
#include "ecs/TagComponent.hpp"

namespace engine {

    template <typename Tuple>
    inline std::shared_ptr<IComponentArray> CreateComponentArrayFromType(const std::type_index& type) {
        static constexpr auto types = get_template_args_array<Tuple>();
        template for (constexpr auto t : types) {
            using Comp = typename [:t:];
            if (type == std::type_index(typeid(Comp))) {
                if constexpr (has_annotation<Integral>(t)) {
                    return std::make_shared<IntegralComponentArray<Comp>>();
                } else {
                    return std::make_shared<ComponentArray<Comp>>();
                }
            }
        }
        throw std::runtime_error("No factory registered for component type: " + std::string(type.name()));
    }

    template <typename Tuple>
    inline std::shared_ptr<SystemBase> CreateSystemFromType(const std::type_index& type, Scene* scene) {
        static constexpr auto types = get_template_args_array<Tuple>();
        template for (constexpr auto t : types) {
            using Sys = typename [:t:];
            if (type == std::type_index(typeid(Sys))) {
                return std::make_shared<Sys>(scene);
            }
        }
        throw std::runtime_error("No factory registered for system type: " + std::string(type.name()));
    }


    Engine::Engine(plt::PlatformInterface* platformInterface, gfx::GraphicsEngine* graphicsEngine,
        am::AssetManagerInterface* assetManagerInterface) : assetManagerInterface(assetManagerInterface), graphicsEngine(graphicsEngine),
                                                            platform(platformInterface)
    {
    }

    void Engine::Initialize()
    {
        platform->SubscribeToEvent(plt::EventType::WindowMinimize,
      [this](const void* /*data*/) {
          minimized = true;
      });

        platform->SubscribeToEvent(plt::EventType::WindowRestored,
            [this](const void* /*data*/) {
                minimized = false;
            });
    }

    std::shared_ptr<Scene> Engine::CreateScene(const std::string& name) {
        auto it = activeScenes.find(name);
        if (it != activeScenes.end()) {
            return it->second.scene; // Scene already exists, return it
        }

        auto scene = std::make_shared<Scene>(*this);
        SceneEntry entry;
        entry.scene = scene;
        entry.updateMode = SceneUpdateMode::Active;
        entry.viewportIndex = 0;
        activeScenes[name] = entry;

        // Optionally set it active if it's the first one
        if (!activeScene) {
            activeScene = scene;
        }

        return scene;
    }

    std::shared_ptr<Scene> Engine::CreateScene(const std::string& name, SceneUpdateMode updateMode, uint32_t viewportIndex) {
        auto it = activeScenes.find(name);
        if (it != activeScenes.end()) {
            it->second.updateMode = updateMode;
            it->second.viewportIndex = viewportIndex;
            if (it->second.scene) {
                it->second.scene->SetTargetViewportIndex(viewportIndex);
            }
            return it->second.scene;
        }

        auto scene = std::make_shared<Scene>(*this);
        scene->SetTargetViewportIndex(viewportIndex);
        SceneEntry entry;
        entry.scene = scene;
        entry.updateMode = updateMode;
        entry.viewportIndex = viewportIndex;
        activeScenes[name] = entry;

        if (!activeScene) {
            activeScene = scene;
        }

        return scene;
    }

    std::shared_ptr<Scene> Engine::GetScene(const std::string& name) {
        auto it = activeScenes.find(name);
        if (it != activeScenes.end()) {
            return it->second.scene;
        }
        return nullptr;
    }

    void Engine::RemoveScene(const std::string& name) {
        auto it = activeScenes.find(name);
        if (it != activeScenes.end()) {
            if (activeScene == it->second.scene) {
                activeScene = nullptr;
            }
            activeScenes.erase(it);
        }
    }

    void Engine::SetActiveScene(const std::string& name) {
        auto it = activeScenes.find(name);
        if (it != activeScenes.end()) {
            activeScene = it->second.scene;
        }
    }

    std::shared_ptr<Scene> Engine::GetActiveScene() {
        return activeScene;
    }

    const std::unordered_map<std::string, SceneEntry>& Engine::GetScenes() const {
        return activeScenes;
    }

    std::unordered_map<std::string, SceneEntry>& Engine::GetScenes() {
        return activeScenes;
    }

    std::optional<SceneEntry> Engine::GetSceneEntry(const std::string& name) const {
        auto it = activeScenes.find(name);
        if (it != activeScenes.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    void Engine::SetSceneUpdateMode(const std::string& name, SceneUpdateMode mode) {
        auto it = activeScenes.find(name);
        if (it != activeScenes.end()) {
            it->second.updateMode = mode;
        }
    }

    void Engine::SetSceneViewportIndex(const std::string& name, uint32_t viewportIndex) {
        auto it = activeScenes.find(name);
        if (it != activeScenes.end()) {
            it->second.viewportIndex = viewportIndex;
            if (it->second.scene) {
                it->second.scene->SetTargetViewportIndex(viewportIndex);
            }
        }
    }

    void Engine::Update(float deltaTime) {
        if (graphicsEngine) {
            graphicsEngine->beginFrame();
        }

        // 1. Update all active scenes
        for (auto& [name, entry] : activeScenes) {
            if (entry.updateMode != SceneUpdateMode::Paused && entry.scene) {
                entry.scene->Update(deltaTime);
            }
        }

        // 2. Render global editor UI / ImGui (if enabled)
        // (ImGui dockspace, model preview windows, etc.)

        if (graphicsEngine) {
            graphicsEngine->endFrame();
        }
    }

    std::shared_ptr<IComponentArray> Engine::CreateComponentArray(const std::type_index& type) const
    {
        return CreateComponentArrayFromType<EngineComponents>(type);
    }

    std::shared_ptr<SystemBase> Engine::CreateSystem(const std::type_index& type, Scene* scene) const
    {
        return CreateSystemFromType<EngineSystems>(type, scene);
    }

    const std::set<std::type_index>& Engine::GetRegisteredComponentTypes() const {
        return GetRegisteredTypesSet<EngineComponents>();
    }

    const std::set<std::type_index>& Engine::GetRegisteredSystemTypes() const {
        return GetRegisteredTypesSet<EngineSystems>();
    }

    std::optional<std::type_index> Engine::GetComponentTypeByName(std::string_view name) const {
        return GetTypeByName<EngineComponents>(name);
    }

    std::optional<std::type_index> Engine::GetSystemTypeByName(std::string_view name) const {
        return GetTypeByName<EngineSystems>(name);
    }

    std::string_view Engine::GetComponentTypeName(const std::type_index& type) const {
        return GetTypeName<EngineComponents>(type);
    }

    std::string_view Engine::GetSystemTypeName(const std::type_index& type) const {
        return GetTypeName<EngineSystems>(type);
    }

    ComponentTypeID Engine::GetComponentTypeID(const std::type_index& type) const {
        return GetTypeIndex<EngineComponents>(type);
    }

    std::type_index Engine::GetComponentTypeFromID(ComponentTypeID id) const {
        return GetTypeByIndex<EngineComponents>(id);
    }

    void Engine::SaveScene()
    {
        am::SceneAsset* sceneAsset = nullptr;
        auto assetInfo = assetManagerInterface->getAssetInfo(activeScene->sceneId);
        if (assetInfo)
        {
            sceneAsset = dynamic_cast<am::SceneAsset*>(assetInfo->get()->getAsset());
            if (!sceneAsset)
            {
                spdlog::error("Failed to cast asset to SceneAsset for scene ID: {}", boost::uuids::to_string(activeScene->sceneId).c_str());
                return;
            }
        }
        else
        {
            assetManagerInterface->createAsset(am::AssetType::Scene, "activeScene");
            assetInfo = assetManagerInterface->getAssetInfo(activeScene->sceneId);
            if (!assetInfo)
            {
                spdlog::error("Failed to create scene asset for scene ID: {}", boost::uuids::to_string(activeScene->sceneId).c_str());
                return;
            }
        }

        rapidjson::Document* document = sceneAsset->getAssetDataAs<rapidjson::Document>();
        if (!document)
        {
            spdlog::error("Scene asset data is null for scene ID: {}", boost::uuids::to_string(activeScene->sceneId).c_str());
            return;
        }

        try
        {
            activeScene->SerializeToJson(*document);
            assetManagerInterface->saveAsset(assetInfo->get()->id);
        } catch (const std::exception& e) {
            spdlog::error("Error saving scene to file: {}", e.what());
        }
    }

    void Engine::LoadScene(boost::uuids::uuid sceneId)
    {
        if (!activeScene)
        {
            activeScene = CreateScene("scene");
        }

        activeScene->sceneId = sceneId;

        auto assetInfo = assetManagerInterface->getAssetInfo(sceneId);
        if (!assetInfo) {
            spdlog::error("Failed to get asset info for scene ID: {}", boost::uuids::to_string(sceneId).c_str());
            return;
        }

        auto sceneAsset = dynamic_cast<am::SceneAsset*>(assetInfo->get()->getAsset());
        if (!sceneAsset) {
            spdlog::error("Asset for scene ID is not a SceneAsset: {}", boost::uuids::to_string(sceneId).c_str());
            return;
        }

        try {


            rapidjson::Document* document = sceneAsset->getAssetDataAs<rapidjson::Document>();
            if (document) {
                activeScene->DeserializeFromJson(*document);
            } else {
                spdlog::error("Scene asset data is null for scene ID: {}", boost::uuids::to_string(sceneId).c_str());
            }

            return ;
        } catch (const std::exception& e) {
            spdlog::error("Error loading scene from file: {}", e.what());
            return;
        }

    }
} // namespace engine