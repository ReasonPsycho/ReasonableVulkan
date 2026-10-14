#include "Engine.h"

#include "PlatformInterface.hpp"
#include "../assetManager/src/assets/engineAssets/SceneAsset.h"
#include "../assetManager/src/assets/engineAssets/PrefabAsset.h"
#include "../assetManager/src/JsonHelpers.hpp"
#include "ecs/Scene.h"
#include "systems/collisionSystem/CollisionSystem.hpp"
#include "systems/editorSystem/EditorSystem.hpp"
#include "systems/gizmoSystem/GizmoSystem.hpp"
#include "systems/renderingSystem/RenderSystem.h"
#include "systems/transformSystem/TransformSystem.h"
#include "systems/scriptSystem/ScriptSystem.hpp"
#include "systems/renderingSystem/componets/MeshComponent.hpp"
#include "systems/renderingSystem/componets/RendererComponent.hpp"
#include "systems/renderingSystem/componets/CameraComponent.hpp"
#include "systems/renderingSystem/componets/LightComponent.hpp"
#include "systems/renderingSystem/componets/ThumbnailComponent.hpp"
#include "systems/transformSystem/componets/TransformComponent.hpp"
#include "systems/scriptSystem/components/ScriptComponent.hpp"
#include "ecs/NameComponent.hpp"
#include "ecs/TagComponent.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <rapidjson/document.h>
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

    Engine::~Engine()
    {
        SaveConfig();
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
        if (assetManagerInterface) {
            assetManagerInterface->setEditor(editorSystem.get());
        }

        LoadConfig();
    }

    std::shared_ptr<editor::EditorInterface> Engine::GetEditor() {
        return editorSystem;
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
        SaveConfig();
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
        for (const auto& scene : activeScenes) {
            if (scene && scene->active) {
                if (auto transformSystem = scene->GetSystem<TransformSystem>()) {
                    transformSystem->Update(deltaTime);
                }
                scene->Render(deltaTime);
            }
        }
        if (graphicsEngine) {
            graphicsEngine->endFrame();
        }
        if (editorSystem) {
            editorSystem->ProcessThumbnailCaptures();
        }
    }

    void Engine::StartPlayMode()
    {
        if (isPlaying) {
            if (isPaused) {
                SetPaused(false);
            }
            return;
        }

        // Save snapshots of all active editable scenes
        sceneSnapshots.clear();
        auto activeScenes = GetActiveScenes();
        for (const auto& scene : activeScenes) {
            if (scene && scene->IsEditable() && scene->GetSceneType() != ecs::SceneType::Thumbnail) {
                rapidjson::Document doc;
                scene->SerializeToJson(doc);
                sceneSnapshots[scene->GetName()] = std::move(doc);
            }
        }

        isPlaying = true;
        isPaused = false;
        if (platform) {
            platform->NotifyIDEState("PLAYING");
        }
        spdlog::info("Entered Play Mode");
    }

    void Engine::StopPlayMode()
    {
        if (!isPlaying) return;

        // 1. Destroy running script instances across scenes
        auto activeScenes = GetActiveScenes();
        for (const auto& scene : activeScenes) {
            if (scene) {
                auto scriptCompArray = scene->GetComponentArray<ScriptComponent>();
                if (scriptCompArray) {
                    auto& comps = scriptCompArray->GetComponents();
                    for (size_t i = 0; i < scriptCompArray->GetArraySize(); ++i) {
                        if (scriptCompArray->IsComponentActive(i)) {
                            for (auto& script : comps[i].scripts) {
                                if (script && script->started) {
                                    script->OnDestroy();
                                    script->started = false;
                                }
                            }
                        }
                    }
                }
            }
        }

        // 2. Restore scenes from snapshots
        for (const auto& [sceneName, snapshot] : sceneSnapshots) {
            auto scene = GetScene(sceneName);
            if (scene) {
                scene->DeserializeFromJson(snapshot);
            }
        }
        sceneSnapshots.clear();

        isPlaying = false;
        isPaused = false;
        if (platform) {
            platform->NotifyIDEState("STOPPED");
        }

        // 3. Validate selected entities in editor
        if (editorSystem) {
            auto target = editorSystem->GetTargetScene();
            if (target) {
                auto sel = editorSystem->GetSelectedEntities();
                std::vector<Entity> validSel;
                for (Entity e : sel) {
                    if (target->IsEntityActive(e)) {
                        validSel.push_back(e);
                    }
                }
                editorSystem->SetSelectedEntities(validSel);
            }
        }

        spdlog::info("Exited Play Mode (Scene restored)");
    }

    void Engine::SetPaused(bool paused)
    {
        if (!isPlaying) return;
        isPaused = paused;
        if (platform) {
            platform->NotifyIDEState(isPaused ? "PAUSED" : "PLAYING");
        }
        spdlog::info(isPaused ? "Play Mode Paused" : "Play Mode Resumed");
    }

    void Engine::TogglePlay()
    {
        if (isPlaying) {
            StopPlayMode();
        } else {
            StartPlayMode();
        }
    }

    void Engine::TogglePause()
    {
        if (!isPlaying) return;
        SetPaused(!isPaused);
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
        if (isPlaying)
        {
            spdlog::warn("Cannot save scene while in Play mode. Exit Play mode to save changes.");
            return;
        }

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
        std::optional<std::shared_ptr<am::AssetInfo>> assetInfo = std::nullopt;
        if (!activeScene->sceneId.is_nil())
        {
            assetInfo = assetManagerInterface->getAssetInfo(activeScene->sceneId);
            if (assetInfo)
            {
                sceneAsset = dynamic_cast<am::SceneAsset*>(assetInfo->get()->getAsset());
                if (!sceneAsset)
                {
                    spdlog::error("Failed to cast asset to SceneAsset for scene ID: {}", boost::uuids::to_string(activeScene->sceneId).c_str());
                    return;
                }
            }
        }

        if (!assetInfo || !sceneAsset)
        {
            std::string savePath;
            if (platform)
            {
                std::string defaultName = activeScene->GetName() + ".scene";
                std::string defaultPath = (std::filesystem::path("res") / defaultName).string();
                auto chosenPath = platform->SaveFileDialog(defaultPath, "Scene Files", "*.scene");
                if (chosenPath.has_value() && !chosenPath.value().empty())
                {
                    savePath = chosenPath.value();
                }
                else
                {
                    spdlog::info("Save scene cancelled by user.");
                    return;
                }
            }
            else
            {
                savePath = (std::filesystem::path("res") / (activeScene->GetName() + ".scene")).string();
            }

            auto newAssetId = assetManagerInterface->createAsset(am::AssetType::Scene, savePath);
            if (!newAssetId.has_value() || newAssetId.value().is_nil())
            {
                spdlog::error("Failed to create scene asset at path: {}", savePath);
                return;
            }

            activeScene->sceneId = newAssetId.value();
            assetInfo = assetManagerInterface->getAssetInfo(activeScene->sceneId);
            if (!assetInfo)
            {
                spdlog::error("Failed to get asset info for scene ID: {}", boost::uuids::to_string(activeScene->sceneId).c_str());
                return;
            }

            sceneAsset = dynamic_cast<am::SceneAsset*>(assetInfo->get()->getAsset());
            if (!sceneAsset)
            {
                spdlog::error("Failed to cast newly created asset to SceneAsset for scene ID: {}", boost::uuids::to_string(activeScene->sceneId).c_str());
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
            SaveConfig();
        } catch (const std::exception& e) {
            spdlog::error("Error saving scene to file: {}", e.what());
        }
    }

    void Engine::LoadScene(boost::uuids::uuid sceneId)
    {
        if (sceneId.is_nil()) return;

        // If a scene with this ID is already loaded, activate and select it
        for (const auto& [name, scn] : scenes) {
            if (scn && scn->sceneId == sceneId) {
                scn->SetActive(true);
                if (editorSystem) {
                    editorSystem->SetTargetScene(scn);
                }
                SaveConfig();
                return;
            }
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

        std::string sceneName = assetInfo->get()->lookUpName;
        if (sceneName.ends_with(".scene")) {
            sceneName = sceneName.substr(0, sceneName.length() - 6);
        }

        auto targetScene = GetScene(sceneName);
        if (!targetScene) {
            targetScene = CreateScene(sceneName);
        }

        targetScene->sceneId = sceneId;
        targetScene->SetActive(true);

        try {
            rapidjson::Document* document = sceneAsset->getAssetDataAs<rapidjson::Document>();
            if (document) {
                targetScene->DeserializeFromJson(*document);
            } else {
                spdlog::error("Scene asset data is null for scene ID: {}", boost::uuids::to_string(sceneId).c_str());
            }

            if (editorSystem) {
                editorSystem->SetTargetScene(targetScene);
            }
            SaveConfig();
            return;
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

    bool Engine::SetupAssetInScene(ecs::Scene* scene, const boost::uuids::uuid& assetId)
    {
        if (!scene || !assetManagerInterface) return false;

        auto assetInfoOpt = assetManagerInterface->getAssetInfo(assetId);
        if (!assetInfoOpt.has_value()) {
            spdlog::error("Asset not found for scene setup: {}", boost::uuids::to_string(assetId));
            return false;
        }

        auto& assetInfo = assetInfoOpt.value();
        if (assetInfo->type == am::AssetType::Model || assetInfo->type == am::AssetType::Mesh)
        {
            auto meshUUID = boost::uuids::nil_uuid();
            glm::vec3 bMin(-1.0f);
            glm::vec3 bMax(1.0f);
            auto assetOpt = assetManagerInterface->getAsset(assetId);
            if (assetOpt.has_value() && assetOpt.value()) {
                if (auto* modelData = assetOpt.value()->getAssetDataAs<am::ModelData>()) {
                    bMin = modelData->boundingBoxMin;
                    bMax = modelData->boundingBoxMax;
                    if (!modelData->rootNode.mChildren.empty() && !modelData->rootNode.mChildren[0].meshes.empty()) {
                        meshUUID = modelData->rootNode.mChildren[0].meshes[0].get()->id;
                    }
                } else if (auto* meshData = assetOpt.value()->getAssetDataAs<am::MeshData>()) {
                    bMin = meshData->boundingBoxMin;
                    bMax = meshData->boundingBoxMax;
                    meshUUID = assetId;
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
            modelTransform.position = -center;
            ecs::Entity modelEntity = scene->CreateEntity(assetInfo->lookUpName, modelTransform);

            MeshComponent meshComp(meshUUID);
            scene->AddComponent<MeshComponent>(modelEntity, meshComp);

            std::string shaderLookup = (assetInfo->type == am::AssetType::Mesh) ? "wiremeshShader" : "pbrShader";
            auto shaderOpt = assetManagerInterface->getAssetUuid(shaderLookup);
            if (shaderOpt) {
                boost::uuids::uuid materialUuid = boost::uuids::nil_uuid();
                auto meshData = assetManagerInterface->getAssetData<am::MeshData>(meshUUID);
                if (meshData && meshData->material) {
                    materialUuid = meshData->material->id;
                }
                RendererComponent rendererComp(shaderOpt.value(), materialUuid);
                scene->AddComponent<RendererComponent>(modelEntity, rendererComp);
            }

            // 2. Create Light Entity
            if (assetInfo->type != am::AssetType::Mesh) {
                TransformComponent lightTransform;
                lightTransform.position = glm::vec3(5.0f, 10.0f, 5.0f);
                lightTransform.rotation = glm::quatLookAt(glm::normalize(glm::vec3(-1.0f, -1.2f, -1.0f)), glm::vec3(0, 1, 0));
                ecs::Entity lightEntity = scene->CreateEntity("Directional Light", lightTransform);

                LightComponent lightComp(LightComponent::Type::Directional, glm::vec3(1.0f, 1.0f, 1.0f), 0.2f);
                scene->AddComponent<LightComponent>(lightEntity, lightComp);
            }

            // 3. Create Camera Entity
            TransformComponent camTransform;
            camTransform.position = glm::vec3(0.5f, maxDim * 0.3f, distance);
            camTransform.rotation = glm::quatLookAt(glm::normalize(-camTransform.position), glm::vec3(0, 1, 0));
            ecs::Entity camEntity = scene->CreateEntity("Main Camera", camTransform);

            CameraComponent camComp;
            camComp.fov = fov;
            camComp.nearPlane = std::max(0.01f, distance * 0.01f);
            camComp.farPlane = distance * 100.0f;
            camComp.active = true;
            camComp.skyboxType = CameraComponent::SkyboxType::Color;
            camComp.skyboxData = SkyboxColorData{glm::vec4(0.2f, 0.2f, 0.2f, 1.0f)};
            scene->AddComponent<CameraComponent>(camEntity, camComp);

            return true;
        }
        else if (assetInfo->type == am::AssetType::Material)
        {
            std::optional<boost::uuids::uuid> sphereUuidOpt = assetManagerInterface->getAssetUuid("internalSphere.model");
            if (!sphereUuidOpt.has_value()) {
                sphereUuidOpt = assetManagerInterface->getAssetUuid("internalSphere");
            }
            if (!sphereUuidOpt.has_value()) {
                sphereUuidOpt = boost::uuids::string_generator()("236cf7ed-1c2a-4fa1-8219-e7dd4ef8a357");
                if (!assetManagerInterface->getAssetInfo(sphereUuidOpt.value()).has_value()) {
                    sphereUuidOpt = std::nullopt;
                }
            }
            if (!sphereUuidOpt.has_value()) {
                sphereUuidOpt = assetManagerInterface->getAssetUuidByPath(std::filesystem::path("internal/internalSphere.model"));
            }
            if (!sphereUuidOpt.has_value()) {
                sphereUuidOpt = assetManagerInterface->getAssetUuidByPath(std::filesystem::path("res/internal/internalSphere.model"));
            }

            if (!sphereUuidOpt.has_value()) {
                spdlog::error("Could not find internalSphere model for material scene");
                return false;
            }

            boost::uuids::uuid sphereId = sphereUuidOpt.value();
            auto sphereAssetOpt = assetManagerInterface->getAsset(sphereId);
            if (!sphereAssetOpt.has_value() || !sphereAssetOpt.value()) {
                spdlog::error("Could not load internalSphere model for material scene");
                return false;
            }

            auto meshUUID = boost::uuids::nil_uuid();
            glm::vec3 bMin(-0.5f);
            glm::vec3 bMax(0.5f);
            if (auto* modelData = sphereAssetOpt.value()->getAssetDataAs<am::ModelData>()) {
                bMin = modelData->boundingBoxMin;
                bMax = modelData->boundingBoxMax;
                if (!modelData->rootNode.mChildren.empty() && !modelData->rootNode.mChildren[0].meshes.empty()) {
                    meshUUID = modelData->rootNode.mChildren[0].meshes[0].get()->id;
                }
            }

            glm::vec3 center = (bMin + bMax) * 0.5f;
            glm::vec3 size = bMax - bMin;
            float maxDim = std::max({size.x, size.y, size.z});
            if (maxDim <= 0.001f) {
                maxDim = 1.0f;
            }

            float fov = 45.0f;
            float distance = (maxDim * 0.5f) / std::sin(glm::radians(fov * 0.5f)) * 1.5f;

            // 1. Create Sphere Entity
            TransformComponent sphereTransform;
            sphereTransform.position = -center;
            ecs::Entity sphereEntity = scene->CreateEntity("Material Preview Sphere", sphereTransform);

            MeshComponent meshComp(meshUUID);
            scene->AddComponent<MeshComponent>(sphereEntity, meshComp);

            auto pbrShader = assetManagerInterface->getAssetUuid("pbrShader");
            if (pbrShader) {
                RendererComponent rendererComp(pbrShader.value(), assetId);
                scene->AddComponent<RendererComponent>(sphereEntity, rendererComp);
            }

            // 2. Create Light Entity
            TransformComponent lightTransform;
            lightTransform.position = glm::vec3(5.0f, 10.0f, 5.0f);
            lightTransform.rotation = glm::quatLookAt(glm::normalize(glm::vec3(-1.0f, -1.2f, -1.0f)), glm::vec3(0, 1, 0));
            ecs::Entity lightEntity = scene->CreateEntity("Directional Light", lightTransform);

            LightComponent lightComp(LightComponent::Type::Directional, glm::vec3(1.0f, 1.0f, 1.0f), 0.2f);
            scene->AddComponent<LightComponent>(lightEntity, lightComp);

            // 3. Create Camera Entity
            TransformComponent camTransform;
            camTransform.position = glm::vec3(0.5f, maxDim * 0.3f, distance);
            camTransform.rotation = glm::quatLookAt(glm::normalize(-camTransform.position), glm::vec3(0, 1, 0));
            ecs::Entity camEntity = scene->CreateEntity("Main Camera", camTransform);

            CameraComponent camComp;
            camComp.fov = fov;
            camComp.nearPlane = std::max(0.01f, distance * 0.01f);
            camComp.farPlane = distance * 100.0f;
            camComp.active = true;
            camComp.skyboxType = CameraComponent::SkyboxType::Color;
            camComp.skyboxData = SkyboxColorData{glm::vec4(0.2f, 0.2f, 0.2f, 1.0f)};
            scene->AddComponent<CameraComponent>(camEntity, camComp);

            return true;
        }

        return false;
    }

    void Engine::SaveConfig()
    {
        if (!assetManagerInterface || configLookupName.empty()) return;

        auto uuid = assetManagerInterface->getAssetUuid(configLookupName);
        if (!uuid) {
            std::filesystem::path configPath = "res/.cache/config/engine.config";
            try {
                uuid = assetManagerInterface->createAsset(am::AssetType::Config, configPath.string(), configLookupName);
            } catch (...) {
                return;
            }
        }
        if (!uuid) return;

        auto configData = assetManagerInterface->getAssetData<rapidjson::Document>(uuid.value());
        if (!configData) return;

        configData->SetObject();
        auto& allocator = configData->GetAllocator();

        rapidjson::Value openedScenesArray(rapidjson::kArrayType);

        for (const auto& sceneName : sceneOrder) {
            auto it = scenes.find(sceneName);
            if (it == scenes.end() || !it->second) continue;
            auto& scn = it->second;
            if (scn->GetSceneType() == ecs::SceneType::Thumbnail) continue;

            rapidjson::Value itemObj(rapidjson::kObjectType);
            itemObj.AddMember("name", rapidjson::Value(scn->GetName().c_str(), allocator), allocator);
            itemObj.AddMember("active", scn->IsActive(), allocator);

            if (scn->IsEditable() && !scn->sceneId.is_nil()) {
                itemObj.AddMember("type", "Scene", allocator);
                std::string idStr = boost::uuids::to_string(scn->sceneId);
                itemObj.AddMember("uuid", rapidjson::Value(idStr.c_str(), allocator), allocator);
                openedScenesArray.PushBack(itemObj, allocator);
            } else if (!scn->IsEditable()) {
                boost::uuids::uuid previewAssetId = boost::uuids::nil_uuid();
                auto meshArray = scn->GetComponentArray<MeshComponent>();
                if (meshArray) {
                    for (int i = 0; i < meshArray->GetArraySize(); ++i) {
                        if (meshArray->IsComponentActive(i)) {
                            previewAssetId = meshArray->GetComponents()[i].meshUuid;
                            break;
                        }
                    }
                }
                if (!previewAssetId.is_nil()) {
                    itemObj.AddMember("type", "AssetPreview", allocator);
                    std::string idStr = boost::uuids::to_string(previewAssetId);
                    itemObj.AddMember("uuid", rapidjson::Value(idStr.c_str(), allocator), allocator);
                    openedScenesArray.PushBack(itemObj, allocator);
                }
            } else {
                itemObj.AddMember("type", "NewScene", allocator);
                openedScenesArray.PushBack(itemObj, allocator);
            }
        }

        configData->AddMember("openedScenes", openedScenesArray, allocator);

        if (editorSystem && editorSystem->GetTargetScene()) {
            std::string selName = editorSystem->GetTargetScene()->GetName();
            configData->AddMember("selectedScene", rapidjson::Value(selName.c_str(), allocator), allocator);
        }

        assetManagerInterface->saveAsset(uuid.value());
    }

    void Engine::LoadConfig()
    {
        if (!assetManagerInterface || configLookupName.empty()) return;

        auto uuid = assetManagerInterface->getAssetUuid(configLookupName);
        if (!uuid) return;

        auto configData = assetManagerInterface->getAssetData<rapidjson::Document>(uuid.value());
        if (!configData || !configData->IsObject()) return;

        bool loadedAny = false;

        if (configData->HasMember("openedScenes") && (*configData)["openedScenes"].IsArray()) {
            const auto& arr = (*configData)["openedScenes"].GetArray();
            for (const auto& item : arr) {
                if (item.IsObject()) {
                    std::string type = item.HasMember("type") && item["type"].IsString() ? item["type"].GetString() : "Scene";
                    std::string idStr = item.HasMember("uuid") && item["uuid"].IsString() ? item["uuid"].GetString() : "";
                    std::string name = item.HasMember("name") && item["name"].IsString() ? item["name"].GetString() : "";
                    bool active = !item.HasMember("active") || !item["active"].IsBool() || item["active"].GetBool();

                    if (type == "Scene" && !idStr.empty()) {
                        try {
                            auto sceneUuid = boost::uuids::string_generator()(idStr);
                            if (!sceneUuid.is_nil()) {
                                LoadScene(sceneUuid);
                                for (auto& [sName, s] : scenes) {
                                    if (s && s->sceneId == sceneUuid) {
                                        s->SetActive(active);
                                        break;
                                    }
                                }
                                loadedAny = true;
                            }
                        } catch (...) {}
                    } else if ((type == "ModelPreview" || type == "AssetPreview") && !idStr.empty()) {
                        try {
                            auto assetUuid = boost::uuids::string_generator()(idStr);
                            if (!assetUuid.is_nil()) {
                                auto scn = editorSystem ? editorSystem->OpenAssetPreviewScene(assetUuid) : nullptr;
                                if (scn) {
                                    scn->SetActive(active);
                                    loadedAny = true;
                                }
                            }
                        } catch (...) {}
                    } else if (type == "NewScene" && !name.empty()) {
                        auto scn = CreateScene(name);
                        if (scn) {
                            scn->SetActive(active);
                            loadedAny = true;
                        }
                    }
                } else if (item.IsString()) {
                    std::string idStr = item.GetString();
                    try {
                        auto sceneUuid = boost::uuids::string_generator()(idStr);
                        if (!sceneUuid.is_nil()) {
                            LoadScene(sceneUuid);
                            loadedAny = true;
                        }
                    } catch (...) {}
                }
            }
        }

        if (configData->HasMember("selectedScene") && (*configData)["selectedScene"].IsString()) {
            std::string selName = (*configData)["selectedScene"].GetString();
            auto scn = GetScene(selName);
            if (scn && editorSystem) {
                editorSystem->SetTargetScene(scn);
            }
        }

        if (!loadedAny && scenes.empty()) {
            auto defaultSceneId = assetManagerInterface->getAssetUuid("scene");
            if (defaultSceneId) {
                LoadScene(defaultSceneId.value());
            }
        }
    }
} // namespace engine