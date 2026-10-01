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
        if (platform) {
            platform->SubscribeToEvent(plt::EventType::WindowMinimize,
          [this](const void* /*data*/) {
              minimized = true;
          });

            platform->SubscribeToEvent(plt::EventType::WindowRestored,
                [this](const void* /*data*/) {
                    minimized = false;
                });
        }

        editorSystem = std::make_shared<ecs::EditorSystem>(this);
        editorSystem->Initialize();
    }

    std::shared_ptr<ecs::EditorSystem> Engine::GetEditorSystem() {
        return editorSystem;
    }

    std::shared_ptr<Scene> Engine::CreateScene(const std::string& name) {
        if (scenes.find(name) != scenes.end()) {
            return scenes[name]; // Scene already exists, return it
        }

        auto scene = std::make_shared<Scene>(*this);  // Changed from (this) to (*this)
        scene->SetName(name);
        scenes[name] = scene;

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
            scenes.erase(it);
        }
    }

    void Engine::SetActiveScene(const std::string& name, bool active) {
        auto it = scenes.find(name);
        if (it != scenes.end()) {
            it->second->active = active;
        }
    }

    void Engine::SetSceneActive(const std::string& name, bool active) {
        SetActiveScene(name, active);
    }

    std::shared_ptr<Scene> Engine::GetActiveScene() {
        for (const auto& [name, scene] : scenes) {
            if (scene && scene->active) {
                return scene;
            }
        }
        return nullptr;
    }

    std::vector<std::shared_ptr<Scene>> Engine::GetActiveScenes() {
        std::vector<std::shared_ptr<Scene>> activeScenes;
        for (const auto& [name, scene] : scenes) {
            if (scene && scene->active) {
                activeScenes.push_back(scene);
            }
        }
        return activeScenes;
    }

    void Engine::Update(float deltaTime) {
        if (graphicsEngine) {
            graphicsEngine->beginFrame();
        }
        auto activeScenes = GetActiveScenes();
        for (const auto& scene : activeScenes) {
            if (scene && scene->active) {
                scene->Update(deltaTime);
            }
        }
        if (editorSystem) {
            editorSystem->Update(deltaTime);
        }
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
        auto activeScene = GetActiveScene();
        if (!activeScene)
        {
            spdlog::error("No active scene to save.");
            return;
        }

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
        auto activeScene = GetActiveScene();
        if (!activeScene)
        {
            activeScene = CreateScene("scene");
            activeScene->active = true;
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