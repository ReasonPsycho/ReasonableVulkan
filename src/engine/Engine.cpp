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
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "../assetManager/include/assetDatas/ModelData.h"
#include "../assetManager/include/assetDatas/MeshData.h"

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
        if (this->assetManagerInterface) {
            this->assetManagerInterface->setEngine(this);
        }
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
        sceneOrder.push_back(name);

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
        std::erase(sceneOrder, name);
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

    std::shared_ptr<Scene> Engine::GetTopEditableScene() {
        for (const auto& name : sceneOrder) {
            auto it = scenes.find(name);
            if (it != scenes.end() && it->second && it->second->active && it->second->IsEditable()) {
                return it->second;
            }
        }
        return nullptr;
    }

    std::shared_ptr<Scene> Engine::GetActiveScene() {
        if (editorSystem) {
            auto selected = editorSystem->GetSelectedScene();
            if (selected && selected->IsActive() && selected->IsEditable()) {
                return selected;
            }
        }
        return GetTopEditableScene();
    }

    std::vector<std::shared_ptr<Scene>> Engine::GetActiveScenes() {
        std::vector<std::shared_ptr<Scene>> activeScenes;
        for (const auto& name : sceneOrder) {
            auto it = scenes.find(name);
            if (it != scenes.end() && it->second && it->second->active) {
                activeScenes.push_back(it->second);
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

        if (!activeScene->IsEditable())
        {
            spdlog::warn("Cannot save non-editable scene: {}", activeScene->GetName());
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

    void Engine::CloseScene(const std::string& name)
    {
        RemoveScene(name);
    }

    void Engine::CloseScene(boost::uuids::uuid sceneId)
    {
        std::string sceneNameToRemove;
        for (const auto& [name, scene] : scenes) {
            if (scene && scene->sceneId == sceneId && !sceneId.is_nil()) {
                sceneNameToRemove = name;
                break;
            }
        }
        if (sceneNameToRemove.empty() && assetManagerInterface) {
            auto assetInfo = assetManagerInterface->getAssetInfo(sceneId);
            if (assetInfo) {
                auto it = scenes.find(assetInfo->get()->lookUpName);
                if (it != scenes.end()) {
                    sceneNameToRemove = it->first;
                }
            }
        }
        if (!sceneNameToRemove.empty()) {
            RemoveScene(sceneNameToRemove);
        }
    }

    void* Engine::GetThumbnailTexture(const boost::uuids::uuid& assetId, const std::string& thumbnailPath)
    {
        if (graphicsEngine) {
            return graphicsEngine->getThumbnailTexture(assetId, thumbnailPath);
        }
        return nullptr;
    }

    bool Engine::CaptureModelThumbnail(const boost::uuids::uuid& modelId, const std::string& outputPath)
    {
        if (!graphicsEngine || !assetManagerInterface) {
            return false;
        }

        auto assetOpt = assetManagerInterface->getAsset(modelId);
        if (!assetOpt.has_value() || !assetOpt.value()) {
            return false;
        }

        am::AssetType type = assetOpt.value()->getType();
        if (type != am::AssetType::Model && type != am::AssetType::Mesh) {
            return false;
        }

        glm::vec3 bMin(-1.0f);
        glm::vec3 bMax(1.0f);
        if (auto* modelData = assetOpt.value()->getAssetDataAs<am::ModelData>()) {
            bMin = modelData->boundingBoxMin;
            bMax = modelData->boundingBoxMax;
        } else if (auto* meshData = assetOpt.value()->getAssetDataAs<am::MeshData>()) {
            bMin = meshData->boundingBoxMin;
            bMax = meshData->boundingBoxMax;
        }

        glm::vec3 center = (bMin + bMax) * 0.5f;
        glm::vec3 size = bMax - bMin;
        float maxDim = std::max({size.x, size.y, size.z});
        if (maxDim <= 0.001f) {
            maxDim = 2.0f;
        }

        float fov = 45.0f;
        float distance = (maxDim * 0.5f) / std::sin(glm::radians(fov * 0.5f)) * 1.5f;

        // Camera at ~45 degrees to the side and above the model
        glm::vec3 camDir = glm::normalize(glm::vec3(1.0f, 0.8f, 1.0f));
        glm::vec3 camPos = center + camDir * distance;
        glm::vec3 camTarget = center;
        glm::vec3 camUp = glm::vec3(0.0f, 1.0f, 0.0f);

        glm::mat4 viewMatrix = glm::lookAt(camPos, camTarget, camUp);
        glm::mat4 projMatrix = glm::perspective(glm::radians(fov), 1.0f, std::max(0.01f, distance * 0.01f), distance * 100.0f);

        glm::vec3 lightDir = glm::normalize(glm::vec3(-1.0f, -1.2f, -1.0f));
        glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
        float lightIntensity = 2.5f;

        return graphicsEngine->renderAndCaptureModelThumbnail(
            modelId, outputPath, 128, 128,
            viewMatrix, projMatrix, camPos,
            lightDir, lightColor, lightIntensity);
    }

    std::shared_ptr<Scene> Engine::OpenModelPreviewScene(const boost::uuids::uuid& modelOrMeshId)
    {
        if (!assetManagerInterface) {
            return nullptr;
        }

        auto assetInfoOpt = assetManagerInterface->getAssetInfo(modelOrMeshId);
        if (!assetInfoOpt.has_value()) {
            spdlog::error("Asset not found for preview: {}", boost::uuids::to_string(modelOrMeshId));
            return nullptr;
        }

        auto& assetInfo = assetInfoOpt.value();
        if (assetInfo->type != am::AssetType::Model && assetInfo->type != am::AssetType::Mesh) {
            spdlog::warn("Asset is not a Model or Mesh: {}", assetInfo->lookUpName);
            return nullptr;
        }

        std::string sceneName = "Preview: " + assetInfo->lookUpName;

        // If scene already exists, activate and select it
        auto it = scenes.find(sceneName);
        if (it != scenes.end()) {
            auto scene = it->second;
            scene->SetActive(true);
            if (editorSystem) {
                editorSystem->SetTargetScene(scene);
                if (!scene->rootEntities.empty()) {
                    editorSystem->SetSelectedEntity(scene->rootEntities[0]);
                }
            }
            return scene;
        }

        auto scene = CreateScene(sceneName);
        scene->SetEditable(false);
        scene->SetActive(true);
        scene->sceneId = boost::uuids::nil_uuid();

        // Calculate model / mesh bounding box
        glm::vec3 bMin(-1.0f);
        glm::vec3 bMax(1.0f);
        auto assetOpt = assetManagerInterface->getAsset(modelOrMeshId);
        if (assetOpt.has_value() && assetOpt.value()) {
            if (auto* modelData = assetOpt.value()->getAssetDataAs<am::ModelData>()) {
                bMin = modelData->boundingBoxMin;
                bMax = modelData->boundingBoxMax;
            } else if (auto* meshData = assetOpt.value()->getAssetDataAs<am::MeshData>()) {
                bMin = meshData->boundingBoxMin;
                bMax = meshData->boundingBoxMax;
            }
        }

        glm::vec3 center = (bMin + bMax) * 0.5f;
        glm::vec3 size = bMax - bMin;
        float maxDim = std::max({size.x, size.y, size.z});
        if (maxDim <= 0.001f) {
            maxDim = 2.0f;
        }

        float fov = 45.0f;
        float distance = (maxDim * 0.5f) / std::sin(glm::radians(fov * 0.5f)) * 1.5f;

        // 1. Create Model / Mesh Entity
        TransformComponent modelTransform;
        modelTransform.position = -center; // Center the model at origin
        ecs::Entity modelEntity = scene->CreateEntity(assetInfo->lookUpName, modelTransform);

        MeshComponent meshComp(modelOrMeshId);
        scene->AddComponent<MeshComponent>(modelEntity, meshComp);

        auto pbrOpt = assetManagerInterface->getAssetUuid("pbrShader");
        if (pbrOpt) {
            RendererComponent rendererComp(pbrOpt.value());
            scene->AddComponent<RendererComponent>(modelEntity, rendererComp);
        }

        // 2. Create Light Entity
        TransformComponent lightTransform;
        lightTransform.position = glm::vec3(5.0f, 10.0f, 5.0f);
        lightTransform.rotation = glm::quatLookAt(glm::normalize(glm::vec3(-1.0f, -1.2f, -1.0f)), glm::vec3(0, 1, 0));
        ecs::Entity lightEntity = scene->CreateEntity("Directional Light", lightTransform);

        LightComponent lightComp(LightComponent::Type::Directional, glm::vec3(1.0f, 1.0f, 1.0f), 2.5f);
        scene->AddComponent<LightComponent>(lightEntity, lightComp);

        // 3. Create Camera Entity
        TransformComponent camTransform;
        camTransform.position = glm::vec3(0.0f, maxDim * 0.3f, distance);
        camTransform.rotation = glm::quatLookAt(glm::normalize(-camTransform.position), glm::vec3(0, 1, 0));
        ecs::Entity camEntity = scene->CreateEntity("Main Camera", camTransform);

        CameraComponent camComp;
        camComp.fov = fov;
        camComp.nearPlane = std::max(0.01f, distance * 0.01f);
        camComp.farPlane = distance * 100.0f;
        camComp.active = true;
        scene->AddComponent<CameraComponent>(camEntity, camComp);

        if (editorSystem) {
            editorSystem->SetTargetScene(scene);
            editorSystem->SetSelectedEntity(modelEntity);
            editorSystem->FocusCameraOnBounds(glm::vec3(0.0f), distance, scene.get());
        }

        return scene;
    }
} // namespace engine