#include "Engine.h"

#include "PlatformInterface.hpp"
#include "../assetManager/src/assets/engineAssets/SceneAsset.h"
#include "ecs/Scene.h"
#include "systems/collisionSystem/CollisionSystem.hpp"
#include "systems/editorSystem/EditorSystem.hpp"
#include "systems/renderingSystem/RenderSystem.h"
#include "systems/transformSystem/TransformSystem.h"

namespace engine {


    Engine::Engine(plt::PlatformInterface* platformInterface, gfx::GraphicsEngine* graphicsEngine,
        am::AssetManagerInterface* assetManagerInterface) : assetManagerInterface(assetManagerInterface), graphicsEngine(graphicsEngine),
                                                            platform(platformInterface)
    {
        RegisterSystemType<RenderSystem>();

#ifdef EDITOR_ENABLED
        RegisterSystemType<EditorSystem>();
#endif

        RegisterComponentType<RendererComponent>(); //For some reason I have to register them in reverse
        RegisterComponentType<CameraComponent>();
        RegisterComponentType<TransformComponent>();
        RegisterComponentType<LightComponent>();

        RegisterSystemType<TransformSystem>();
        RegisterSystemType<CollisionSystem>();
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

    std::shared_ptr<IComponentArray> Engine::CreateComponentArray(const std::type_index& type) const
    {
        auto it = componentFactories.find(type);
        if (it != componentFactories.end()) {
            return it->second();
        }
        throw std::runtime_error("No factory registered for component type: " + std::string(type.name()));
    }

    std::shared_ptr<SystemBase> Engine::CreateSystem(const std::type_index& type, Scene* scene) const
    {
        auto it = systemFactories.find(type);
        if (it != systemFactories.end()) {
            return it->second(scene);
        }
        throw std::runtime_error("No factory registered for system type: " + std::string(type.name()));
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
        if (activeScene) {
            activeScene->sceneId = sceneId;
        }

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