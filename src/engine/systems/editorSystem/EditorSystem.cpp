//
// Created by redkc on 09/10/2025.
//

#include "EditorSystem.hpp"
#include <ImGuizmo.h>

#include "ecs/Scene.h"
#include "Engine.h"
#include <SDL3/SDL_mouse.h>

#include "Asset.hpp"
#include "systems/renderingSystem/componets/CameraComponent.hpp"
#include "systems/renderingSystem/componets/MeshComponent.hpp"
#include "systems/renderingSystem/componets/RendererComponent.hpp"
#include "systems/renderingSystem/componets/LightComponent.hpp"
#include "systems/renderingSystem/componets/ThumbnailComponent.hpp"
#include "systems/transformSystem/componets/TransformComponent.hpp"
#include "systems/scriptSystem/components/ScriptComponent.hpp"
#include "systems/scriptSystem/ScriptSystem.hpp"
#include "systems/scriptSystem/ScriptHotReloadManager.hpp"
#include "systems/scriptSystem/ScriptRegistry.hpp"
#include <thread>
#include <fstream>
#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#endif
#include "ecs/NameComponent.hpp"
#include "ecs/TagComponent.hpp"
#include "PlatformInterface.hpp"
#include "assetDatas/MeshData.h"
#include "assetDatas/ModelData.h"
#include "systems/collisionSystem/CollisionSystem.hpp"
#include "systems/gizmoSystem/GizmoSystem.hpp"
#include "RenderDocManager.hpp"
#include "assetManager/src/assets/engineAssets/PrefabAsset.h"
#include "assetManager/src/JsonHelpers.hpp"
#include "ComponentGizmoRegistry.hpp"
#include "visualizers/LightGizmoVisualizer.hpp"
#include "visualizers/CameraGizmoVisualizer.hpp"
#include "IconsFontAwesome6.h"
#include "systems/renderingSystem/componets/CameraComponent.hpp"

namespace engine::ecs {

static std::vector<std::filesystem::path> ParseFilePathPayload(const ImGuiPayload* payload) {
    std::vector<std::filesystem::path> paths;
    if (!payload || !payload->Data || payload->DataSize == 0) return paths;
    const char* str = (const char*)payload->Data;
    std::stringstream ss(str);
    std::string line;
    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) {
            paths.push_back(std::filesystem::path(line));
        }
    }
    return paths;
}

static std::vector<EntityID> ParseSceneEntityPayload(const ImGuiPayload* payload) {
    std::vector<EntityID> ents;
    if (!payload || !payload->Data || payload->DataSize == 0) return ents;
    if (payload->DataSize >= sizeof(engine::ecs::SceneEntityPayload)) {
        const auto* pl = (const engine::ecs::SceneEntityPayload*)payload->Data;
        if (pl->count > 0) {
            for (uint32_t k = 0; k < pl->count; ++k) ents.push_back(pl->entities[k]);
        }
    }
    if (ents.empty()) {
        ents.push_back(*(const EntityID*)payload->Data);
    }
    return ents;
}

Scene* EditorSystem::GetTargetScene() const
{
    if (auto scn = selectedScene.lock()) {
        return scn.get();
    }
    if (engine) {
        auto topEditable = engine->GetTopEditableScene();
        if (topEditable) return topEditable.get();

        auto activeScenes = engine->GetActiveScenes();
        for (const auto& scn : activeScenes) {
            if (scn && scn->GetSceneType() != ecs::SceneType::Thumbnail) {
                return scn.get();
            }
        }
    }
    return nullptr;
}

void EditorSystem::ImGuiInspector()
{
    if (requestInspectorFocus)
    {
        ImGui::SetNextWindowFocus();
        requestInspectorFocus = false;
    }
    ImGui::Begin("Inspector");
    Scene* scene = GetTargetScene();
    if (!scene || selectedEntities.empty())
    {
        ImGui::TextDisabled("No entity selected");
        ImGui::End();
        return;
    }

    bool isEditable = scene->IsEditable();
    if (!isEditable)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f), "Non-Editable Scene (Read-Only)");
        ImGui::Separator();
        ImGui::BeginDisabled(true);
    }

    if (selectedEntities.size() > 1)
    {
        bool allActive = true;
        for (Entity e : selectedEntities) {
            if (!scene->IsEntityActive(e)) {
                allActive = false;
                break;
            }
        }
        bool activeToggle = allActive;
        if (ImGui::Checkbox("##AllActiveToggle", &activeToggle))
        {
            std::vector<std::pair<Entity, bool>> oldStates;
            for (Entity e : selectedEntities) {
                oldStates.push_back({e, scene->IsEntityActive(e)});
            }
            auto cmd = std::make_shared<editor::SetEntityActiveCommand>(scene, oldStates, activeToggle);
            history.Execute(cmd);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle Active State for All Selected Entities");
        ImGui::SameLine();
        ImGui::Text("%zu Entities Selected", selectedEntities.size());

        ImGui::Separator();

        if (ImGui::CollapsingHeader("Selection Actions", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Button("Save Selected as Prefabs"))
            {
                for (Entity e : selectedEntities) {
                    SaveEntityAsPrefab(e, "res/prefabs", scene);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear Selection"))
            {
                ClearSelectedEntities();
                if (!isEditable) ImGui::EndDisabled();
                ImGui::End();
                return;
            }

            if (ImGui::Button("Copy Selected (Ctrl+C)"))
            {
                CopySelectedEntities(scene);
            }
            ImGui::SameLine();
            if (ImGui::Button("Duplicate Selected (Ctrl+D)"))
            {
                DuplicateSelectedEntities(scene);
            }

            if (HasEntitiesInClipboard())
            {
                ImGui::SameLine();
                if (ImGui::Button("Paste (Ctrl+V)"))
                {
                    PasteEntities(scene);
                }
            }

            ImGui::Separator();
            if (ImGui::Button("Delete All Selected (Del)"))
            {
                auto entsToDelete = selectedEntities;
                auto cmd = std::make_shared<editor::DeleteEntitiesCommand>(scene, entsToDelete, this);
                history.Execute(cmd);
                if (!isEditable) ImGui::EndDisabled();
                ImGui::End();
                return;
            }
        }

        if (ImGui::CollapsingHeader("Selected Entities List", ImGuiTreeNodeFlags_DefaultOpen))
        {
            Entity toRemove = std::numeric_limits<std::uint32_t>::max();
            Entity toSolo = std::numeric_limits<std::uint32_t>::max();
            for (Entity e : selectedEntities)
            {
                ImGui::PushID((int)e);
                std::string eName = GetEntityName(e, scene);
                if (ImGui::Selectable(eName.c_str(), false))
                {
                    toSolo = e;
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Click to select only this entity");
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20.0f);
                if (ImGui::SmallButton("x"))
                {
                    toRemove = e;
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Remove from selection");
                ImGui::PopID();
            }
            if (toSolo != std::numeric_limits<std::uint32_t>::max())
            {
                SetSelectedEntity(toSolo);
            }
            else if (toRemove != std::numeric_limits<std::uint32_t>::max())
            {
                RemoveSelectedEntity(toRemove);
            }
        }

        if (!isEditable)
        {
            ImGui::EndDisabled();
        }
        ImGui::End();
        return;
    }

    Entity selectedEntity = selectedEntities[0];
    if (!scene->IsEntityActive(selectedEntity) && !scene->HasComponent<TransformComponent>(selectedEntity))
    {
        ClearSelectedEntities();
        if (!isEditable) ImGui::EndDisabled();
        ImGui::End();
        return;
    }

    // 1. Entity Header: Active toggle, Name, Entity ID
        bool active = scene->IsEntityActive(selectedEntity);
        if (ImGui::Checkbox("##EntityActive", &active))
        {
            auto cmd = std::make_shared<editor::SetEntityActiveCommand>(scene, std::vector<std::pair<Entity, bool>>{{selectedEntity, !active}}, active);
            history.Execute(cmd);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip(active ? "Entity is Active" : "Entity is Inactive");
        ImGui::SameLine();

        char nameBuf[256];
        std::string rawName = GetEntityRawName(selectedEntity, scene);
        std::snprintf(nameBuf, sizeof(nameBuf), "%s", rawName.c_str());
        float idWidth = 70.0f;
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - idWidth);
        if (ImGui::InputText("##EntityName", nameBuf, sizeof(nameBuf), ImGuiInputTextFlags_EnterReturnsTrue) || ImGui::IsItemDeactivatedAfterEdit())
        {
            if (rawName != nameBuf) {
                auto cmd = std::make_shared<editor::RenameEntityCommand>(this, scene, selectedEntity, rawName, nameBuf);
                history.Execute(cmd);
            }
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Entity Name (Editable)");
        ImGui::SameLine();
        ImGui::TextDisabled("ID: %u", selectedEntity);

        // 2. Tag row (TagComponent)
        if (scene->HasComponent<TagComponent>(selectedEntity))
        {
            auto& tagComp = scene->GetComponent<TagComponent>(selectedEntity);
            char tagBuf[128];
            std::snprintf(tagBuf, sizeof(tagBuf), "%s", tagComp.tag.c_str());
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 30.0f);
            if (ImGui::InputText("Tag", tagBuf, sizeof(tagBuf)))
            {
                tagComp.tag = tagBuf;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("x##RemoveTag"))
            {
                scene->RemoveComponent<TagComponent>(selectedEntity);
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Remove TagComponent");
        }
        else
        {
            if (ImGui::SmallButton("+ Add Tag"))
            {
                scene->AddComponent<TagComponent>(selectedEntity, TagComponent{"Untagged"});
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add TagComponent to entity");
        }

        // Entity actions row: Copy, Duplicate, Paste
        if (ImGui::SmallButton("Copy (Ctrl+C)"))
        {
            CopySelectedEntities(scene);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Copy Entity to Clipboard (Ctrl+C)");
        ImGui::SameLine();
        if (ImGui::SmallButton("Duplicate (Ctrl+D)"))
        {
            DuplicateSelectedEntities(scene);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Duplicate Entity (Ctrl+D)");

        if (HasEntitiesInClipboard())
        {
            ImGui::SameLine();
            if (ImGui::SmallButton("Paste (Ctrl+V)"))
            {
                PasteEntities(scene);
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Paste Entities from Clipboard (Ctrl+V)");
        }

        ImGui::Separator();

        // 3. Components list
        auto componentArrays = scene->GetComponentArrays();

        // Always display TransformComponent first if present
        if (scene->HasComponent<TransformComponent>(selectedEntity))
        {
            auto transformArray = scene->GetIntegralComponentArray<TransformComponent>();
            if (transformArray)
            {
                registeredComponentTypes[typeid(TransformComponent)].showImGuiComponent(scene, transformArray->GetComponentUntyped(selectedEntity));
            }
        }

        // Draw other components (excluding NameComponent, TransformComponent, TagComponent)
        std::type_index componentToRemove = typeid(void);
        for (auto& [typeIndex, array] : componentArrays)
        {
            if (typeIndex == typeid(NameComponent) || typeIndex == typeid(TransformComponent) || typeIndex == typeid(TagComponent))
            {
                continue;
            }

            if (array->HasComponentUntyped(selectedEntity))
            {
                if (registeredComponentTypes[typeIndex].showImGuiComponent(scene, array->GetComponentUntyped(selectedEntity)))
                {
                    componentToRemove = typeIndex;
                }
            }
        }

        if (componentToRemove != typeid(void))
        {
            auto cmd = std::make_shared<editor::RemoveComponentCommand>(scene, selectedEntity, componentToRemove, registeredComponentTypes[componentToRemove].displayName);
            history.Execute(cmd);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 4. Add Component button
        float buttonWidth = 200.0f;
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - buttonWidth) * 0.5f);
        if (ImGui::Button("Add Component", ImVec2(buttonWidth, 0)))
        {
            ImGui::OpenPopup("Components List");
        }

        if (ImGui::BeginPopup("Components List"))
        {
            bool anyAvailable = false;
            for (const auto& [typeIndex, info] : registeredComponentTypes)
            {
                // Skip integral components (TransformComponent, NameComponent)
                if (info.isIntegral || typeIndex == typeid(TransformComponent) || typeIndex == typeid(NameComponent))
                {
                    continue;
                }

                auto it = componentArrays.find(typeIndex);
                bool alreadyHas = (it != componentArrays.end() && it->second->HasComponentUntyped(selectedEntity));

                if (!alreadyHas)
                {
                    anyAvailable = true;
                    if (ImGui::MenuItem(info.displayName.c_str()))
                    {
                        auto cmd = std::make_shared<editor::AddComponentCommand>(scene, selectedEntity, typeIndex, info.displayName);
                        history.Execute(cmd);
                    }
                }
            }

            if (!anyAvailable)
            {
                ImGui::TextDisabled("No more components available");
            }

            auto registeredScripts = ScriptRegistry::Instance().GetRegisteredNames();
            if (!registeredScripts.empty())
            {
                if (ImGui::BeginMenu("Scripts"))
                {
                    for (const auto& scriptName : registeredScripts)
                    {
                        if (ImGui::MenuItem(scriptName.c_str()))
                        {
                            if (!scene->HasComponent<ScriptComponent>(selectedEntity))
                            {
                                auto cmd = std::make_shared<editor::AddComponentCommand>(scene, selectedEntity, typeid(ScriptComponent), "ScriptComponent");
                                history.Execute(cmd);
                            }
                            if (scene->HasComponent<ScriptComponent>(selectedEntity))
                            {
                                auto& scriptComp = scene->GetComponent<ScriptComponent>(selectedEntity);
                                auto script = scriptComp.AddScriptByName(scriptName);
                                if (script) {
                                    script->scene = scene;
                                    script->entity = selectedEntity;
                                    script->OnAwake();
                                }
                            }
                        }
                    }
                    ImGui::EndMenu();
                }
            }

            ImGui::EndPopup();
        }

        if (!isEditable)
        {
            ImGui::EndDisabled();
        }

        ImGui::End();
    }

void EditorSystem::ImGuiSystemSettings()
{
    ImGui::Begin("System Settings");
    Scene* scene = GetTargetScene();
    if (scene)
    {
        for (const auto& [typeIndex, system] : scene->GetSystems())
        {
            if (ImGui::CollapsingHeader(system->name.c_str()))
            {
                system->DrawSettingsImGui(scene);
            }
        }
    }
    else
    {
        ImGui::TextDisabled("No active scene");
    }
    ImGui::End();
}

void EditorSystem::ImGuiGizmoForScene(Scene* scene, EditorCameraState& camState, const ImVec2& viewportPos, const ImVec2& viewportSize)
{
    if (!scene || !scene->IsEditable())
    {
        return;
    }

    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);

    float viewMatrix[16], projMatrix[16];
    memcpy(viewMatrix, &camState.camera.view[0][0], sizeof(float) * 16);
    memcpy(projMatrix, &camState.camera.projection[0][0], sizeof(float) * 16);

    // Draw Grid using ImGuizmo
    if (showGrid)
    {
        float identityMatrix[16];
        glm::mat4 idMat(1.0f);
        memcpy(identityMatrix, &idMat[0][0], sizeof(float) * 16);
        ImGuizmo::DrawGrid(viewMatrix, projMatrix, identityMatrix, gridSize);
    }

    static ImGuizmo::OPERATION currentGizmoOperation(ImGuizmo::ROTATE);
    static ImGuizmo::MODE currentGizmoMode(ImGuizmo::WORLD);
    static bool useSnap = false;

    // Keyboard shortcuts for operation changes
    if (!ImGui::GetIO().WantTextInput)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_T))
            currentGizmoOperation = ImGuizmo::TRANSLATE;
        if (ImGui::IsKeyPressed(ImGuiKey_E))
            currentGizmoOperation = ImGuizmo::ROTATE;
        if (ImGui::IsKeyPressed(ImGuiKey_R))
            currentGizmoOperation = ImGuizmo::SCALE;
        if (ImGui::IsKeyPressed(ImGuiKey_S))
            useSnap = !useSnap;
        if (ImGui::IsKeyPressed(ImGuiKey_G))
            showGrid = !showGrid;
    }

    // Tool Overlay Window in Top Right
    {
        const float PADDING = 1.0f;

        ImVec2 window_pos = ImVec2(viewportPos.x + viewportSize.x - PADDING, viewportPos.y + PADDING + 37);
        ImVec2 window_pos_pivot = ImVec2(1.0f, 0.0f);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

        if (ImGui::Begin("GizmoTools", nullptr, window_flags))
        {
            // Translate
            if (ImGui::RadioButton("T", currentGizmoOperation == ImGuizmo::TRANSLATE))
                currentGizmoOperation = ImGuizmo::TRANSLATE;
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Translate (T)");
            ImGui::SameLine();

            // Rotate
            if (ImGui::RadioButton("R", currentGizmoOperation == ImGuizmo::ROTATE))
                currentGizmoOperation = ImGuizmo::ROTATE;
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Rotate (E)");
            ImGui::SameLine();

            // Scale
            if (ImGui::RadioButton("S", currentGizmoOperation == ImGuizmo::SCALE))
                currentGizmoOperation = ImGuizmo::SCALE;
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Scale (R)");
            ImGui::SameLine();

            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical, 3.0f);
            ImGui::SameLine();

            // Mode selection (except for Scale)
            if (currentGizmoOperation != ImGuizmo::SCALE)
            {
                if (ImGui::RadioButton("L", currentGizmoMode == ImGuizmo::LOCAL))
                    currentGizmoMode = ImGuizmo::LOCAL;
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Local Mode");
                ImGui::SameLine();
                if (ImGui::RadioButton("W", currentGizmoMode == ImGuizmo::WORLD))
                    currentGizmoMode = ImGuizmo::WORLD;
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("World Mode");
                ImGui::SameLine();
            }

            // Snap toggle
            if (ImGui::Checkbox("##Snap", &useSnap))
            {
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Use Snap (S)");
            ImGui::SameLine();

            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical, 3.0f);
            ImGui::SameLine();

            // Grid toggle
            if (ImGui::Checkbox("Grid", &showGrid))
            {
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle Grid (G)");
            ImGui::SameLine();

            // Component Gizmo visualizers toggles (discovered via C++26 reflection over EngineComponents)
            ComponentGizmoRegistry<>::DrawToolbarToggles();
        }
        ImGui::End();
    }

    // Draw component gizmos (lights, cameras, etc.) via C++26 reflection
    {
        GizmoContext ctx;
        ctx.drawList = ImGui::GetWindowDrawList();
        ctx.viewProj = camState.camera.projection * camState.camera.view;
        ctx.viewportPos = viewportPos;
        ctx.viewportSize = viewportSize;
        ctx.mousePos = ImGui::GetIO().MousePos;

        ComponentGizmoRegistry<>::DrawAllGizmos(scene, ctx, [this](Entity e) {
            return IsEntitySelected(e);
        });
    }

    EntityID targetEntity = GetSelectedEntity();
    if (targetEntity != std::numeric_limits<std::uint32_t>::max() && scene->HasComponent<TransformComponent>(targetEntity))
    {
        auto& transform = scene->GetIntegralComponentArray<TransformComponent>().get()->GetComponentFromEntity(targetEntity);

        auto isDescendant = [scene](EntityID e, EntityID ancestor) -> bool {
            if (!scene || e == MAX_ENTITIES || ancestor == MAX_ENTITIES) return false;
            EntityID current = scene->GetParent(e);
            while (current != MAX_ENTITIES && current != (EntityID)-1) {
                if (current == ancestor) return true;
                current = scene->GetParent(current);
            }
            return false;
        };

        std::vector<EntityID> topLevelSelected;
        for (EntityID e : selectedEntities) {
            if (!scene->HasComponent<TransformComponent>(e)) continue;
            bool hasSelectedAncestor = false;
            for (EntityID other : selectedEntities) {
                if (other != e && isDescendant(e, other)) {
                    hasSelectedAncestor = true;
                    break;
                }
            }
            if (!hasSelectedAncestor) {
                if (std::find(topLevelSelected.begin(), topLevelSelected.end(), e) == topLevelSelected.end()) {
                    topLevelSelected.push_back(e);
                }
            }
        }
        if (std::find(topLevelSelected.begin(), topLevelSelected.end(), targetEntity) == topLevelSelected.end()) {
            topLevelSelected.push_back(targetEntity);
        }

        auto transformArray = scene->GetIntegralComponentArray<TransformComponent>().get();

        // Calculate center (middle) of all selected objects
        glm::vec3 center(0.0f);
        int validCount = 0;
        if (transformArray) {
            for (EntityID e : topLevelSelected) {
                if (transformArray->HasComponent(e)) {
                    const auto& t = transformArray->GetComponentFromEntity(e);
                    center += getGlobalPosition(t);
                    validCount++;
                }
            }
        }
        if (validCount > 0) {
            center /= static_cast<float>(validCount);
        } else {
            center = getGlobalPosition(transform);
        }

        glm::mat4 gizmoMatrix;
        if (topLevelSelected.size() <= 1) {
            gizmoMatrix = transform.globalMatrix;
        } else {
            if (currentGizmoMode == ImGuizmo::LOCAL) {
                gizmoMatrix = glm::translate(glm::mat4(1.0f), center) * glm::toMat4(getGlobalRotation(transform));
            } else {
                gizmoMatrix = glm::translate(glm::mat4(1.0f), center);
            }
        }

        static bool s_GizmoWasUsing = false;
        static EntityID s_GizmoTargetEntity = MAX_ENTITIES;
        static Scene* s_GizmoTargetScene = nullptr;
        static glm::mat4 s_GizmoInitialMatrix{1.0f};
        static glm::mat4 s_GizmoCurrentMatrix{1.0f};
        static std::vector<editor::EntityTransformRecord> s_GizmoInitialRecords;

        bool isGizmoUsing = ImGuizmo::IsUsing();

        if (isGizmoUsing && !s_GizmoWasUsing)
        {
            s_GizmoTargetEntity = targetEntity;
            s_GizmoTargetScene = scene;
            s_GizmoInitialMatrix = gizmoMatrix;
            s_GizmoCurrentMatrix = gizmoMatrix;
            s_GizmoInitialRecords.clear();
            if (transformArray) {
                for (EntityID e : topLevelSelected) {
                    if (transformArray->HasComponent(e)) {
                        auto& t = transformArray->GetComponentFromEntity(e);
                        editor::EntityTransformRecord rec;
                        rec.entity = e;
                        rec.oldLocalMatrix = t.localMatrix;
                        rec.oldGlobalMatrix = t.globalMatrix;
                        rec.newLocalMatrix = t.localMatrix;
                        rec.newGlobalMatrix = t.globalMatrix;
                        s_GizmoInitialRecords.push_back(rec);
                    }
                }
            }
        }

        if (!isGizmoUsing)
        {
            s_GizmoCurrentMatrix = gizmoMatrix;
        }

        glm::vec3 snap(1.0f);
        if (currentGizmoOperation == ImGuizmo::TRANSLATE)
            snap = glm::vec3(0.5f); // Snap every 0.5 units for translation
        else if (currentGizmoOperation == ImGuizmo::ROTATE)
            snap = glm::vec3(45.0f); // Snap every 45 degrees for rotation
        else if (currentGizmoOperation == ImGuizmo::SCALE)
            snap = glm::vec3(0.1f); // Snap every 0.1 units for scale

        float modelMatrix[16];
        memcpy(modelMatrix, &s_GizmoCurrentMatrix[0][0], sizeof(float) * 16);

        // Manipulate the transform
        if (ImGuizmo::Manipulate(
            viewMatrix,
            projMatrix,
            currentGizmoOperation,
            currentGizmoMode,
            modelMatrix,
            nullptr,
            useSnap ? &snap[0] : nullptr))
        {
            // Convert the manipulated matrix back to our transform
            glm::mat4 newGizmoMatrix;
            memcpy(&newGizmoMatrix[0][0], modelMatrix, sizeof(float) * 16);
            s_GizmoCurrentMatrix = newGizmoMatrix;

            glm::mat4 deltaMatrix = newGizmoMatrix * glm::inverse(s_GizmoInitialMatrix);

            if (transformArray) {
                for (Entity e : topLevelSelected) {
                    if (!transformArray->HasComponent(e)) continue;
                    auto& eTransform = transformArray->GetComponentFromEntity(e);

                    glm::mat4 eInitialGlobal = eTransform.globalMatrix;
                    for (const auto& rec : s_GizmoInitialRecords) {
                        if (rec.entity == e) {
                            eInitialGlobal = rec.oldGlobalMatrix;
                            break;
                        }
                    }
                    glm::mat4 eNewGlobal = deltaMatrix * eInitialGlobal;

                    Entity p = scene->GetParent(e);
                    if (p != MAX_ENTITIES)
                    {
                        auto& parentTransform = transformArray->GetComponentFromEntity(p);
                        setLocalMatrixFromGlobal(eTransform, eNewGlobal, parentTransform.globalMatrix);
                    }
                    else
                    {
                        setLocalMatrix(eTransform, eNewGlobal);
                    }
                }
            }
        }

        if (!isGizmoUsing && s_GizmoWasUsing)
        {
            if (s_GizmoTargetScene == scene && s_GizmoTargetEntity == targetEntity)
            {
                auto transformArray = scene->GetIntegralComponentArray<TransformComponent>().get();
                if (transformArray) {
                    std::vector<editor::EntityTransformRecord> changedRecords;
                    for (auto rec : s_GizmoInitialRecords) {
                        if (transformArray->HasComponent(rec.entity)) {
                            auto& curT = transformArray->GetComponentFromEntity(rec.entity);
                            if (curT.localMatrix != rec.oldLocalMatrix) {
                                rec.newLocalMatrix = curT.localMatrix;
                                rec.newGlobalMatrix = curT.globalMatrix;
                                changedRecords.push_back(rec);
                            }
                        }
                    }
                    if (!changedRecords.empty()) {
                        auto cmd = std::make_shared<editor::EntitiesTransformCommand>(scene, changedRecords);
                        history.Push(cmd);
                    }
                }
            }
            s_GizmoTargetEntity = MAX_ENTITIES;
            s_GizmoTargetScene = nullptr;
            s_GizmoInitialRecords.clear();
        }

        s_GizmoWasUsing = isGizmoUsing;
    }
}

void EditorSystem::ImguiToolbar()
{
    // Create the windows
    ImGui::Begin("Toolbar");

    auto& rdoc = rd::RenderDocManager::getInstance();
    if (rdoc.isApiAvailable()) {
        if (rdoc.isCapturing() || rdoc.isCaptureRequested()) {
            ImGui::BeginDisabled(true);
            ImGui::Button("Capturing Frame...");
            ImGui::EndDisabled();
        } else {
            if (ImGui::Button("Capture Frame (F11)")) {
                rdoc.requestCapture();
            }
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("Trigger a RenderDoc frame capture on the next frame (Hotkey: F11)");
        }
    } else {
        ImGui::BeginDisabled(true);
        ImGui::Button("Capture Frame (F11)");
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("RenderDoc is not attached. Launch the application from RenderDoc to enable capturing.");
        }
    }

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical, 2.0f);
    ImGui::SameLine();

    bool canUndo = CanUndo();
    ImGui::BeginDisabled(!canUndo);
    if (ImGui::Button(ICON_FA_ROTATE_LEFT " Undo")) {
        Undo();
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        if (canUndo) {
            ImGui::SetTooltip("Undo: %s (Ctrl+Z)", history.GetUndoName().c_str());
        } else {
            ImGui::SetTooltip("Nothing to undo (Ctrl+Z)");
        }
    }

    ImGui::SameLine();
    bool canRedo = CanRedo();
    ImGui::BeginDisabled(!canRedo);
    if (ImGui::Button(ICON_FA_ROTATE_RIGHT " Redo")) {
        Redo();
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        if (canRedo) {
            ImGui::SetTooltip("Redo: %s (Ctrl+Shift+Z / Ctrl+Y)", history.GetRedoName().c_str());
        } else {
            ImGui::SetTooltip("Nothing to redo (Ctrl+Shift+Z / Ctrl+Y)");
        }
    }

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical, 2.0f);
    ImGui::SameLine();

    if (isScriptCompiling.load()) {
        ImGui::BeginDisabled(true);
        ImGui::Button(ICON_FA_SPINNER " Compiling Scripts...");
        ImGui::EndDisabled();
    } else {
        if (ImGui::Button(ICON_FA_ROTATE " Recompile Scripts (Ctrl+R)")) {
            TriggerScriptRebuild();
        }
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("Asynchronously compile and hot-reload gameplay scripts (Ctrl+R)");
    }

    if (!lastScriptBuildOutput.empty()) {
        ImGui::SameLine();
        if (lastScriptBuildSuccess) {
            ImGui::TextColored(ImVec4(0.2f, 0.85f, 0.2f, 1.0f), "[Reloaded]");
        } else {
            ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "[Build Error]");
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Log")) {
            showScriptBuildLogWindow = true;
        }
    }

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical, 2.0f);
    ImGui::SameLine();

    bool isPlaying = engine ? engine->IsPlaying() : false;
    bool isPaused = engine ? engine->IsPaused() : false;

    // Play button
    if (isPlaying) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.65f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.75f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.55f, 0.2f, 1.0f));
    }
    if (ImGui::Button(isPlaying ? (ICON_FA_PLAY " Playing") : (ICON_FA_PLAY " Play"))) {
        if (engine) engine->TogglePlay();
    }
    if (isPlaying) {
        ImGui::PopStyleColor(3);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(isPlaying ? "Stop Game (Ctrl+P)" : "Play Game (Ctrl+P)");
    }

    ImGui::SameLine();

    // Pause button
    ImGui::BeginDisabled(!isPlaying);
    if (isPaused) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.6f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.7f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.75f, 0.5f, 0.1f, 1.0f));
    }
    if (ImGui::Button(isPaused ? (ICON_FA_PAUSE " Paused") : (ICON_FA_PAUSE " Pause"))) {
        if (engine) engine->TogglePause();
    }
    if (isPaused) {
        ImGui::PopStyleColor(3);
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip(isPaused ? "Resume Game (Ctrl+Shift+P)" : "Pause Game (Ctrl+Shift+P)");
    }

    ImGui::SameLine();

    // Stop button
    ImGui::BeginDisabled(!isPlaying);
    if (ImGui::Button(ICON_FA_STOP " Stop")) {
        if (engine) engine->StopPlayMode();
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("Stop Game and reset scene state");
    }

    ImGui::End();
}

EditorSystem::EditorSystem(::engine::Engine* engine) : engine(engine)
{
    lastProcessedStateTimestamp = 0;
}

bool EditorSystem::IsPlaying() const {
    return engine ? engine->IsPlaying() : false;
}

bool EditorSystem::IsPaused() const {
    return engine ? engine->IsPaused() : false;
}

void EditorSystem::StartPlayMode() {
    if (engine) engine->StartPlayMode();
}

void EditorSystem::StopPlayMode() {
    if (engine) engine->StopPlayMode();
}

void EditorSystem::SetPaused(bool paused) {
    if (engine) engine->SetPaused(paused);
}

void EditorSystem::TogglePlay() {
    if (engine) engine->TogglePlay();
}

void EditorSystem::TogglePause() {
    if (engine) engine->TogglePause();
}

void EditorSystem::ImguiShaderOverrideWindow(const ImVec2& viewportPos, const ImVec2& viewportSize)
{
    const float PADDING = 1.0f;

    ImVec2 window_pos = ImVec2(viewportPos.x + viewportSize.x - PADDING, viewportPos.y + PADDING );
    ImVec2 window_pos_pivot = ImVec2(1.0f, 0.0f);
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

    ImGui::Begin("Shader Override", nullptr, window_flags);
    const char* items[] = { "Default", "Wiremesh", "Textured Wiremesh" };
    int current = (int)currentShaderOverride;
    if (ImGui::Combo("Mode", &current, items, IM_ARRAYSIZE(items))) {
        currentShaderOverride = (ShaderOverrideMode)current;
    }
    ImGui::End();
}

void EditorSystem::Update(float deltaTime)
{
    if (!engine || engine->minimized)
        return;

    PollPluginBridge();

    if (ImGui::IsKeyPressed(ImGuiKey_F11)) {
        rd::RenderDocManager::getInstance().requestCapture();
    }

    // Keyboard shortcuts for Undo (Ctrl+Z) and Redo (Ctrl+Shift+Z / Ctrl+Y), Copy (Ctrl+C), Paste (Ctrl+V), Duplicate (Ctrl+D)
    bool isCtrlOrCmd = ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeySuper;
    bool isShift = ImGui::GetIO().KeyShift;

    if (!ImGui::GetIO().WantTextInput)
    {
        if (isCtrlOrCmd && !isShift && ImGui::IsKeyPressed(ImGuiKey_Z))
        {
            Undo();
        }
        if ((isCtrlOrCmd && isShift && ImGui::IsKeyPressed(ImGuiKey_Z)) ||
            (isCtrlOrCmd && ImGui::IsKeyPressed(ImGuiKey_Y)))
        {
            Redo();
        }
        if (isCtrlOrCmd && !isShift && ImGui::IsKeyPressed(ImGuiKey_C))
        {
            CopySelectedEntities();
        }
        if (isCtrlOrCmd && !isShift && ImGui::IsKeyPressed(ImGuiKey_V))
        {
            PasteEntities();
        }
        if (isCtrlOrCmd && !isShift && ImGui::IsKeyPressed(ImGuiKey_D))
        {
            DuplicateSelectedEntities();
        }
        if (isCtrlOrCmd && !isShift && ImGui::IsKeyPressed(ImGuiKey_R))
        {
            TriggerScriptRebuild();
        }
        if (isCtrlOrCmd && !isShift && ImGui::IsKeyPressed(ImGuiKey_P))
        {
            if (engine) engine->TogglePlay();
        }
        if (isCtrlOrCmd && isShift && ImGui::IsKeyPressed(ImGuiKey_P))
        {
            if (engine) engine->TogglePause();
        }
    }

    if (scriptCompilePendingReload.load()) {
        scriptCompilePendingReload = false;
        spdlog::info("EditorSystem: Triggering hot reload across active scenes...");
        if (engine) {
            auto activeScenes = engine->GetActiveScenes();
            struct SceneSnapshot {
                std::shared_ptr<ScriptSystem> system;
                rapidjson::Document doc;
            };
            std::vector<SceneSnapshot> snapshots;
            for (auto& scn : activeScenes) {
                if (scn) {
                    if (auto ss = scn->GetSystem<ScriptSystem>()) {
                        SceneSnapshot snap;
                        snap.system = ss;
                        snap.doc = ss->BeginHotReload();
                        snapshots.push_back(std::move(snap));
                    }
                }
            }
            bool reloadSuccess = ScriptHotReloadManager::Instance().ReloadScriptLibrary();
            for (auto& snap : snapshots) {
                if (snap.system) {
                    snap.system->EndHotReload(snap.doc);
                }
            }
            if (reloadSuccess) {
                spdlog::info("EditorSystem: Hot reload completed successfully across active scenes");
            } else {
                spdlog::warn("EditorSystem: Script library reload failed");
            }
        } else {
            auto targetScene = GetTargetScene();
            if (targetScene) {
                ScriptSystem::PerformHotReloadForScene(targetScene);
            } else {
                ScriptHotReloadManager::Instance().ReloadScriptLibrary();
            }
        }
    }

    // Create the docking space with transparent background
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                                  ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                  ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");

    // Set up the default docking layout (only once)
    static bool first_time = true;
    if (first_time)
    {
        first_time = false;
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

        // Define the splitting setup
        ImGuiID dock_main_id = dockspace_id;
        ImGuiID dock_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.2f, nullptr, &dock_main_id);
        ImGuiID dock_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.2f, nullptr, &dock_main_id);
        ImGuiID dock_top = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Up, 0.1f, nullptr, &dock_main_id);
        ImGuiID dock_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.1f, nullptr, &dock_main_id);

        // Dock the windows
        ImGui::DockBuilderDockWindow("Scene graph", dock_left);
        ImGui::DockBuilderDockWindow("Inspector", dock_right);
        ImGui::DockBuilderDockWindow("File Inspector", dock_right);
        ImGui::DockBuilderDockWindow("System Settings", dock_right);
        ImGui::DockBuilderDockWindow("History", dock_right);
        ImGui::DockBuilderDockWindow("Toolbar", dock_top);
        ImGui::DockBuilderDockWindow("Menu", dock_bottom);
        ImGui::DockBuilderDockWindow("Editor", dock_main_id);
        ImGui::DockBuilderDockWindow("Game", dock_main_id);

        if (engine) {
            auto currentActiveScenes = engine->GetActiveScenes();
            for (const auto& scn : currentActiveScenes) {
                if (scn && scn->GetSceneType() != ecs::SceneType::Thumbnail) {
                    ImGui::DockBuilderDockWindow(("Editor (" + scn->GetName() + ")").c_str(), dock_main_id);
                }
            }
        }

        ImGui::DockBuilderFinish(dockspace_id);
    }

    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    // 1. Render all Editor windows (one per active scene)
    auto activeScenes = engine->GetActiveScenes();
    std::string sceneToCloseFromEditorWindow;
    for (size_t sceneIdx = 0; sceneIdx < activeScenes.size(); ++sceneIdx) {
        auto currentScene = activeScenes[sceneIdx];
        if (!currentScene || currentScene->GetSceneType() == ecs::SceneType::Thumbnail) continue;

        std::string windowTitle = (activeScenes.size() == 1) ? "Editor" : ("Editor (" + currentScene->GetName() + ")");
        ImGuiDockNode* centralNode = ImGui::DockBuilderGetCentralNode(dockspace_id);
        ImGuiID centerId = centralNode ? centralNode->ID : dockspace_id;
        ImGui::SetNextWindowDockID(centerId, ImGuiCond_FirstUseEver);

        bool isSceneWindowOpen = true;
        ImGui::Begin(windowTitle.c_str(), &isSceneWindowOpen);
        if (!isSceneWindowOpen) {
            sceneToCloseFromEditorWindow = currentScene->GetName();
        }
        ImGuizmo::SetDrawlist();
        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        ImVec2 viewportPos = ImGui::GetWindowPos();
        ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
        viewportPos.x += contentMin.x;
        viewportPos.y += contentMin.y;

        auto& camState = GetEditorCameraState(currentScene.get());
        camState.lastViewportPos = viewportPos;
        lastViewportPos = viewportPos;

        if (viewportPanelSize.x != camState.lastViewportSize.x || viewportPanelSize.y != camState.lastViewportSize.y)
        {
            camState.lastViewportSize = viewportPanelSize;
            lastViewportSize = viewportPanelSize;
        }

        uint32_t camIdx = static_cast<uint32_t>(sceneIdx);
        void* textureId = engine->graphicsEngine ? engine->graphicsEngine->getViewportTexturePointer(camIdx) : nullptr;
        if (textureId) {
            ImGui::Image((ImTextureID)textureId, viewportPanelSize);

            ImGuiIO& io = ImGui::GetIO();
            bool isHovered = ImGui::IsItemHovered();

            if (isHovered && (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle))) {
                selectedScene = currentScene;
            }

            // Zoom
            if (isHovered && io.MouseWheel != 0.0f) {
                float zoomSensitivity = 0.1f;
                camState.cameraDistance = glm::max(0.1f, camState.cameraDistance - io.MouseWheel * zoomSensitivity);
                camState.UpdateCameraPosition();
            }

            // Selection (Left Click & Box Drag Select)
            if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsOver() && !ImGuizmo::IsUsing()) {
                selectedScene = currentScene;
                camState.isBoxSelecting = true;
                camState.boxSelectStart = io.MousePos;
                camState.preBoxSelectedEntities = selectedEntities;
            }

            if (camState.isBoxSelecting) {
                float dragDist = std::hypot(io.MousePos.x - camState.boxSelectStart.x, io.MousePos.y - camState.boxSelectStart.y);
                if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    if (dragDist > 4.0f) {
                        ImVec2 bMin = ImVec2(std::min(camState.boxSelectStart.x, io.MousePos.x), std::min(camState.boxSelectStart.y, io.MousePos.y));
                        ImVec2 bMax = ImVec2(std::max(camState.boxSelectStart.x, io.MousePos.x), std::max(camState.boxSelectStart.y, io.MousePos.y));

                        // Clamp drawing to viewport rect
                        ImVec2 clipMin = viewportPos;
                        ImVec2 clipMax = ImVec2(viewportPos.x + viewportPanelSize.x, viewportPos.y + viewportPanelSize.y);
                        ImVec2 drawBoxMin = ImVec2(std::max(bMin.x, clipMin.x), std::max(bMin.y, clipMin.y));
                        ImVec2 drawBoxMax = ImVec2(std::min(bMax.x, clipMax.x), std::min(bMax.y, clipMax.y));

                        if (drawBoxMax.x > drawBoxMin.x && drawBoxMax.y > drawBoxMin.y) {
                            ImDrawList* drawList = ImGui::GetWindowDrawList();
                            drawList->AddRectFilled(drawBoxMin, drawBoxMax, IM_COL32(66, 150, 250, 45));
                            drawList->AddRect(drawBoxMin, drawBoxMax, IM_COL32(66, 150, 250, 220), 0.0f, 0, 1.5f);
                        }

                        // Determine which entities are inside the marquee box
                        std::vector<EntityID> inBoxEntities;
                        auto transformArray = currentScene->GetIntegralComponentArray<TransformComponent>().get();
                        auto meshArray = currentScene->GetComponentArray<MeshComponent>().get();
                        if (transformArray) {
                            auto& transforms = transformArray->GetComponents();
                            for (EntityID e = 0; e < MAX_ENTITIES; ++e) {
                                if (!transformArray->HasComponent(e)) continue;
                                if (!currentScene->IsEntityActive(e)) continue;

                                bool overlaps = false;
                                // If entity has a mesh component with bounding box, check projected 2D AABB
                                if (meshArray && meshArray->HasComponent(e)) {
                                    auto& meshComp = meshArray->GetComponentFromEntity(e);
                                    if (meshComp.meshUuid != boost::uuids::nil_uuid()) {
                                        auto meshData = engine->assetManagerInterface ?
                                            engine->assetManagerInterface->getAssetData<am::MeshData>(meshComp.meshUuid) : nullptr;
                                        if (meshData) {
                                            glm::vec3 bmin = meshData->boundingBoxMin;
                                            glm::vec3 bmax = meshData->boundingBoxMax;
                                            glm::vec3 corners[8] = {
                                                glm::vec3(transforms[e].globalMatrix * glm::vec4(bmin.x, bmin.y, bmin.z, 1.0f)),
                                                glm::vec3(transforms[e].globalMatrix * glm::vec4(bmax.x, bmin.y, bmin.z, 1.0f)),
                                                glm::vec3(transforms[e].globalMatrix * glm::vec4(bmin.x, bmax.y, bmin.z, 1.0f)),
                                                glm::vec3(transforms[e].globalMatrix * glm::vec4(bmax.x, bmax.y, bmin.z, 1.0f)),
                                                glm::vec3(transforms[e].globalMatrix * glm::vec4(bmin.x, bmin.y, bmax.z, 1.0f)),
                                                glm::vec3(transforms[e].globalMatrix * glm::vec4(bmax.x, bmin.y, bmax.z, 1.0f)),
                                                glm::vec3(transforms[e].globalMatrix * glm::vec4(bmin.x, bmax.y, bmax.z, 1.0f)),
                                                glm::vec3(transforms[e].globalMatrix * glm::vec4(bmax.x, bmax.y, bmax.z, 1.0f))
                                            };
                                            float minScreenX = std::numeric_limits<float>::max();
                                            float maxScreenX = -std::numeric_limits<float>::max();
                                            float minScreenY = std::numeric_limits<float>::max();
                                            float maxScreenY = -std::numeric_limits<float>::max();
                                            bool anyFront = false;

                                            for (int c = 0; c < 8; ++c) {
                                                glm::vec4 clip = camState.camera.projection * camState.camera.view * glm::vec4(corners[c], 1.0f);
                                                if (clip.w > 0.0f) {
                                                    anyFront = true;
                                                    glm::vec3 ndc = glm::vec3(clip) / clip.w;
                                                    float sx = viewportPos.x + (ndc.x + 1.0f) * 0.5f * viewportPanelSize.x;
                                                    float sy = viewportPos.y + (1.0f - ndc.y) * 0.5f * viewportPanelSize.y;
                                                    minScreenX = std::min(minScreenX, sx);
                                                    maxScreenX = std::max(maxScreenX, sx);
                                                    minScreenY = std::min(minScreenY, sy);
                                                    maxScreenY = std::max(maxScreenY, sy);
                                                }
                                            }

                                            if (anyFront) {
                                                overlaps = (bMin.x <= maxScreenX && bMax.x >= minScreenX &&
                                                            bMin.y <= maxScreenY && bMax.y >= minScreenY);
                                            }
                                        }
                                    }
                                }

                                if (!overlaps) {
                                    // Fallback to testing entity's origin/position
                                    glm::vec3 worldPos = glm::vec3(transforms[e].globalMatrix[3]);
                                    glm::vec4 clip = camState.camera.projection * camState.camera.view * glm::vec4(worldPos, 1.0f);
                                    if (clip.w > 0.0f) {
                                        glm::vec3 ndc = glm::vec3(clip) / clip.w;
                                        float sx = viewportPos.x + (ndc.x + 1.0f) * 0.5f * viewportPanelSize.x;
                                        float sy = viewportPos.y + (1.0f - ndc.y) * 0.5f * viewportPanelSize.y;
                                        overlaps = (sx >= bMin.x && sx <= bMax.x && sy >= bMin.y && sy <= bMax.y);
                                    }
                                }

                                if (overlaps) {
                                    inBoxEntities.push_back(e);
                                }
                            }
                        }

                        bool isCtrl = io.KeyCtrl || io.KeySuper;
                        bool isShift = io.KeyShift;

                        if (isCtrl) {
                            std::vector<EntityID> combined = camState.preBoxSelectedEntities;
                            for (EntityID e : inBoxEntities) {
                                auto it = std::find(combined.begin(), combined.end(), e);
                                if (it != combined.end()) combined.erase(it);
                                else combined.push_back(e);
                            }
                            SetSelectedEntities(combined);
                        } else if (isShift) {
                            std::vector<EntityID> combined = camState.preBoxSelectedEntities;
                            for (EntityID e : inBoxEntities) {
                                if (std::find(combined.begin(), combined.end(), e) == combined.end()) {
                                    combined.push_back(e);
                                }
                            }
                            SetSelectedEntities(combined);
                        } else {
                            SetSelectedEntities(inBoxEntities);
                        }
                    }
                } else {
                    // Left mouse button released
                    if (dragDist <= 4.0f) {
                        GizmoContext clickCtx;
                        clickCtx.viewProj = camState.camera.projection * camState.camera.view;
                        clickCtx.viewportPos = viewportPos;
                        clickCtx.viewportSize = viewportPanelSize;
                        clickCtx.mousePos = camState.boxSelectStart;

                        auto hitGizmoEntity = ComponentGizmoRegistry<>::HitTestAllGizmos(currentScene.get(), clickCtx, 16.0f);

                        bool isCtrl = io.KeyCtrl || io.KeySuper;
                        bool isShift = io.KeyShift;

                        if (hitGizmoEntity.has_value() && *hitGizmoEntity != MAX_ENTITIES)
                        {
                            Entity hitEntity = *hitGizmoEntity;
                            if (isCtrl) {
                                if (IsEntitySelected(hitEntity)) {
                                    RemoveSelectedEntity(hitEntity);
                                } else {
                                    AddSelectedEntity(hitEntity);
                                }
                            } else if (isShift) {
                                AddSelectedEntity(hitEntity);
                            } else {
                                SetSelectedEntity(hitEntity);
                            }
                        }
                        else
                        {
                            // Single click raycast selection
                            auto* collisionSystem = currentScene->GetSystem<CollisionSystem>().get();
                            if (collisionSystem) {
                                ImVec2 mousePos = camState.boxSelectStart;
                                Ray ray = collisionSystem->ScreenToWorldRay(camState.camera, mousePos.x - viewportPos.x,
                                                                            mousePos.y - viewportPos.y, viewportPanelSize.x, viewportPanelSize.y);

                                /*auto gizmoSystem = currentScene->GetSystem<GizmoSystem>();
                                if (gizmoSystem) {
                                    gizmoSystem->DrawRay(ray.origin, ray.direction * 100.0f, glm::vec3(1.0f, 0.0f, 0.0f), 5);
                                }*/

                                auto hit = collisionSystem->RayCastClosest(ray);

                                if (hit.has_value()) {
                                    if (isCtrl) {
                                        if (IsEntitySelected(hit->entity)) {
                                            RemoveSelectedEntity(hit->entity);
                                        } else {
                                            AddSelectedEntity(hit->entity);
                                        }
                                    } else if (isShift) {
                                        AddSelectedEntity(hit->entity);
                                    } else {
                                        SetSelectedEntity(hit->entity);
                                    }
                                } else {
                                    if (!isCtrl && !isShift) {
                                        ClearSelectedEntities();
                                    }
                                }
                            }
                        }
                    }
                    camState.isBoxSelecting = false;
                }
            }

            // Orbit/Pan Start
            if (isHovered) {
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) camState.isRightMousePressed = true;
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) camState.isMiddleMousePressed = true;
            }

            // Orbit/Pan Stop
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Right)) camState.isRightMousePressed = false;
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Middle)) camState.isMiddleMousePressed = false;

            // Orbit camera
            if (camState.isRightMousePressed) {
                float sensitivity = 0.3f;
                camState.cameraYaw -= io.MouseDelta.x * sensitivity;
                camState.cameraPitch += io.MouseDelta.y * sensitivity;
                camState.cameraPitch = glm::clamp(camState.cameraPitch, -89.0f, 89.0f);
                camState.UpdateCameraPosition();
            }

            // Pan camera
            if (camState.isMiddleMousePressed) {
                float sensitivity = 0.001f * camState.cameraDistance;
                glm::vec3 back = glm::normalize(camState.cameraTransform.position - camState.cameraTarget);
                glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), back));
                glm::vec3 up = glm::cross(back, right);

                camState.cameraTarget += right * (-io.MouseDelta.x * sensitivity);
                camState.cameraTarget += up * (io.MouseDelta.y * sensitivity);

                camState.UpdateCameraPosition();
            }

            // Gizmos for this scene if target
            if (currentScene.get() == GetTargetScene()) {
                ImGuiGizmoForScene(currentScene.get(), camState, viewportPos, viewportPanelSize);
                ImguiShaderOverrideWindow(viewportPos, viewportPanelSize);
            }

            // Drag and drop asset into viewport
            if (currentScene->IsEditable())
            {
                ImVec2 vpMin = viewportPos;
                ImVec2 vpMax = ImVec2(viewportPos.x + viewportPanelSize.x, viewportPos.y + viewportPanelSize.y);
                bool isVpDropTarget = false;
                std::string draggedAssetName;
                std::string draggedAssetExt;

                if (ImGui::BeginDragDropTarget())
                {
                    isVpDropTarget = true;
                    const ImGuiPayload* curPayload = ImGui::GetDragDropPayload();
                    if (curPayload && curPayload->IsDataType("AM_FILE_PATH") && curPayload->Data) {
                        auto droppedPaths = ParseFilePathPayload(curPayload);
                        if (droppedPaths.size() > 1) {
                            draggedAssetName = std::to_string(droppedPaths.size()) + " items";
                            draggedAssetExt.clear();
                        } else if (!droppedPaths.empty()) {
                            draggedAssetName = droppedPaths[0].filename().string();
                            draggedAssetExt = droppedPaths[0].extension().string();
                        }
                    }

                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AM_FILE_PATH"))
                    {
                        auto droppedPaths = ParseFilePathPayload(payload);
                        std::vector<EntityID> createdEntities;
                        for (const auto& p : droppedPaths)
                        {
                            auto ext = p.extension().string();
                            if (ext == ".scene") {
                                auto uuid = engine->assetManagerInterface->getAssetUuidByPath(p);
                                if (uuid) engine->LoadScene(uuid.value());
                            } else if (ext == ".prefab") {
                                auto uuid = engine->assetManagerInterface->getAssetUuidByPath(p);
                                if (!uuid) uuid = engine->assetManagerInterface->registerAsset(p.string());
                                if (uuid) {
                                    Entity newEnt = currentScene->InstantiatePrefab(uuid.value());
                                    if (newEnt != MAX_ENTITIES) {
                                        createdEntities.push_back(newEnt.GetID());
                                    }
                                }
                            } else if (ext == ".fbx" || ext == ".obj" || ext == ".model" || ext == ".mesh" || ext == ".gltf" || ext == ".glb" || ext == ".blend" || ext == ".blend1") {
                                auto uuid = engine->assetManagerInterface->getAssetUuidByPath(p);
                                if (!uuid) uuid = engine->assetManagerInterface->registerAsset(p.string());
                                if (uuid) {
                                    Entity newEnt = currentScene->InstantiateModel(uuid.value());
                                    if (newEnt != MAX_ENTITIES) {
                                        createdEntities.push_back(newEnt.GetID());
                                    }
                                }
                            }
                        }
                        if (!createdEntities.empty()) {
                            auto cmd = std::make_shared<editor::InstantiateEntitiesCommand>(currentScene.get(), createdEntities, draggedAssetName.empty() ? "Asset" : draggedAssetName, this);
                            history.Push(cmd);
                            SetSelectedEntities(createdEntities);
                            selectedScene = currentScene->engine.GetScene(currentScene->GetName());
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                if (isVpDropTarget)
                {
                    ImDrawList* fgDrawList = ImGui::GetForegroundDrawList();
                    fgDrawList->AddRect(vpMin, vpMax, IM_COL32(66, 180, 255, 230), 0.0f, 0, 3.0f);
                    fgDrawList->AddRectFilled(vpMin, vpMax, IM_COL32(66, 150, 250, 35));

                    std::string badgeText;
                    if (draggedAssetExt == ".scene") {
                        badgeText = "Drop to load scene: " + draggedAssetName;
                    } else if (draggedAssetExt == ".prefab") {
                        badgeText = "Drop to instantiate prefab: " + draggedAssetName;
                    } else if (draggedAssetExt == ".fbx" || draggedAssetExt == ".obj" || draggedAssetExt == ".model" || draggedAssetExt == ".mesh" || draggedAssetExt == ".gltf" || draggedAssetExt == ".glb" || draggedAssetExt == ".blend" || draggedAssetExt == ".blend1") {
                        badgeText = "Drop to instantiate model: " + draggedAssetName;
                    } else if (!draggedAssetName.empty()) {
                        badgeText = "Drop asset: " + draggedAssetName;
                    } else {
                        badgeText = "Drop asset into viewport";
                    }

                    ImVec2 textSize = ImGui::CalcTextSize(badgeText.c_str());
                    ImVec2 badgePad(16.0f, 10.0f);
                    ImVec2 centerPos(vpMin.x + (viewportPanelSize.x - textSize.x) * 0.5f, vpMin.y + (viewportPanelSize.y - textSize.y) * 0.5f);
                    ImVec2 bMin(centerPos.x - badgePad.x, centerPos.y - badgePad.y);
                    ImVec2 bMax(centerPos.x + textSize.x + badgePad.x, centerPos.y + textSize.y + badgePad.y);

                    fgDrawList->AddRectFilled(bMin, bMax, IM_COL32(20, 25, 35, 230), 6.0f);
                    fgDrawList->AddRect(bMin, bMax, IM_COL32(66, 180, 255, 255), 6.0f, 0, 1.5f);
                    fgDrawList->AddText(centerPos, IM_COL32(255, 255, 255, 255), badgeText.c_str());
                }
            }
        }
        ImGui::End();
    }

    if (!sceneToCloseFromEditorWindow.empty()) {
        if (auto currentSelected = selectedScene.lock()) {
            if (currentSelected->GetName() == sceneToCloseFromEditorWindow) {
                selectedScene.reset();
                ClearSelectedEntities();
                renamingEntity = std::numeric_limits<std::uint32_t>::max();
            }
        }
        engine->CloseScene(sceneToCloseFromEditorWindow);
    }

    // 2. Render ONE Game window for the top editable active scene
    auto topEditableScene = engine->GetTopEditableScene();
    if (topEditableScene) {
        auto cameraArray = topEditableScene->GetComponentArray<CameraComponent>();
        bool foundActive = false;
        if (cameraArray) {
            auto& cameras = cameraArray->GetComponents();
            for (int i = 0; i < cameraArray->GetArraySize(); i++) {
                if (cameraArray->IsComponentActive(i) && cameras[i].active) {
                    foundActive = true;
                    break;
                }
            }
        }

        if (foundActive) {
            ImGuiDockNode* centralNode = ImGui::DockBuilderGetCentralNode(dockspace_id);
            ImGuiID centerId = centralNode ? centralNode->ID : dockspace_id;
            ImGui::SetNextWindowDockID(centerId, ImGuiCond_FirstUseEver);
            ImGui::Begin("Game");
            ImVec2 gameViewportPanelSize = ImGui::GetContentRegionAvail();
            uint32_t gameCamIdx = static_cast<uint32_t>(activeScenes.size());
            void* gameTextureId = engine->graphicsEngine ? engine->graphicsEngine->getViewportTexturePointer(gameCamIdx) : nullptr;
            if (gameTextureId && gameViewportPanelSize.x > 0.0f && gameViewportPanelSize.y > 0.0f) {
                float camAspect = 1.7777778f;
                if (cameraArray) {
                    auto& cameras = cameraArray->GetComponents();
                    for (int i = 0; i < cameraArray->GetArraySize(); i++) {
                        if (cameraArray->IsComponentActive(i) && cameras[i].active && cameras[i].aspectRatio > 0.0f) {
                            camAspect = cameras[i].aspectRatio;
                            break;
                        }
                    }
                }

                float panelAspect = gameViewportPanelSize.x / gameViewportPanelSize.y;
                ImVec2 imageSize = gameViewportPanelSize;
                if (panelAspect > camAspect) {
                    imageSize.x = gameViewportPanelSize.y * camAspect;
                    imageSize.y = gameViewportPanelSize.y;
                } else {
                    imageSize.x = gameViewportPanelSize.x;
                    imageSize.y = gameViewportPanelSize.x / camAspect;
                }

                ImVec2 offset = ImVec2((gameViewportPanelSize.x - imageSize.x) * 0.5f, (gameViewportPanelSize.y - imageSize.y) * 0.5f);
                if (offset.x > 0.0f || offset.y > 0.0f) {
                    ImGui::SetCursorPos(ImGui::GetCursorPos() + offset);
                }
                ImGui::Image((ImTextureID)gameTextureId, imageSize);
            }
            ImGui::End();
        }
    }

    ImguiToolbar();

    if (engine->assetManagerInterface && engine->assetManagerInterface->imguiFileBrowser) {
        engine->assetManagerInterface->imguiFileBrowser->ImguiFileBrowser("Menu");
        engine->assetManagerInterface->imguiFileBrowser->ImguiFileInspector("File Inspector");
    }

    ImGuiSceneGraph();
    ImGuiInspector();
    ImGuiSystemSettings();
    ImGuiHistoryWindow("History");
    ImGuiScriptBuildLogWindow();

    ImGui::End();
}

void EditorSystem::ProcessThumbnailCaptures()
{
    if (!engine || !engine->graphicsEngine) return;

    auto activeScenes = engine->GetActiveScenes();
    std::vector<std::string> scenesToRemove;

    for (size_t s = 0; s < activeScenes.size(); ++s) {
        auto scene = activeScenes[s];
        if (!scene || !scene->active) continue;

        auto thumbArray = scene->GetComponentArray<ThumbnailComponent>();
        if (!thumbArray) continue;

        for (int i = 0; i < thumbArray->GetArraySize(); ++i) {
            if (thumbArray->IsComponentActive(i)) {
                auto& thumbComp = thumbArray->GetComponents()[i];
                if (thumbComp.framesRendered < 1) {
                    continue;
                }

                if (!thumbComp.outputPath.empty()) {
                    uint32_t camIdx = static_cast<uint32_t>(s);
                    engine->graphicsEngine->captureOffscreenImage(camIdx, thumbComp.outputPath, thumbComp.targetWidth, thumbComp.targetHeight);

                    if (engine->assetManagerInterface && !thumbComp.assetId.is_nil()) {
                        auto assetInfoOpt = engine->assetManagerInterface->getAssetInfo(thumbComp.assetId);
                        if (assetInfoOpt && assetInfoOpt.value()) {
                            assetInfoOpt.value()->thumbnailPath = ".cache/thumbnails/" + boost::uuids::to_string(thumbComp.assetId) + ".png";
                            engine->assetManagerInterface->saveAssetMetadata(thumbComp.assetId);
                        }
                    }

                    thumbComp.captured = true;
                }

                if (scene->GetSceneType() == SceneType::Thumbnail) {
                    scenesToRemove.push_back(scene->GetName());
                }
            }
        }
    }

    for (const auto& sceneName : scenesToRemove) {
        engine->RemoveScene(sceneName);
    }
}

void EditorSystem::SetEntityName(EntityID entity, const std::string& name, Scene* targetScene)
{
    Scene* scene = targetScene ? targetScene : GetTargetScene();
    if (!scene) return;

    if (scene->HasComponent<NameComponent>(entity)) {
        scene->GetComponent<NameComponent>(entity).name = name;
    } else if (!name.empty()) {
        scene->AddComponent<NameComponent>(entity, NameComponent{name});
    }

    if (name.empty()) {
        named_entities.erase(entity);
    } else {
        named_entities[entity] = name;
    }
}

std::string EditorSystem::GetEntityRawName(EntityID entity, Scene* targetScene) const
{
    Scene* scene = targetScene ? targetScene : GetTargetScene();
    if (scene && scene->HasComponent<NameComponent>(entity)) {
        const auto& compName = scene->GetComponent<NameComponent>(entity).name;
        if (!compName.empty()) {
            return compName;
        }
    }
    auto it = named_entities.find(entity);
    return it != named_entities.end() ? it->second : "Entity";
}

std::string EditorSystem::GetEntityName(EntityID entity, Scene* targetScene) const
{
    return "(#" + std::to_string(entity) + ") " + GetEntityRawName(entity, targetScene);
}

void EditorSystem::CopySelectedEntities(Scene* sourceScene)
{
    Scene* scene = sourceScene ? sourceScene : GetTargetScene();
    if (!scene || selectedEntities.empty()) return;

    auto isDescendant = [scene](EntityID e, EntityID ancestor) -> bool {
        if (!scene || e == MAX_ENTITIES || ancestor == MAX_ENTITIES) return false;
        EntityID current = scene->GetParent(e);
        while (current != MAX_ENTITIES && current != (EntityID)-1) {
            if (current == ancestor) return true;
            current = scene->GetParent(current);
        }
        return false;
    };

    std::vector<EntityID> topLevelSelected;
    for (EntityID e : selectedEntities) {
        if (!scene->IsEntityActive(e) && !scene->HasComponent<TransformComponent>(e)) continue;
        bool hasSelectedAncestor = false;
        for (EntityID other : selectedEntities) {
            if (other != e && isDescendant(e, other)) {
                hasSelectedAncestor = true;
                break;
            }
        }
        if (!hasSelectedAncestor) {
            if (std::find(topLevelSelected.begin(), topLevelSelected.end(), e) == topLevelSelected.end()) {
                topLevelSelected.push_back(e);
            }
        }
    }

    if (topLevelSelected.empty()) return;

    entityClipboard.clear();
    for (EntityID e : topLevelSelected) {
        EntityClipboardItem item;
        item.name = GetEntityRawName(e, scene);
        item.originalParent = scene->GetParent(e);
        item.doc = std::make_shared<rapidjson::Document>();
        scene->SerializeObjectToJson(e, *item.doc);
        entityClipboard.push_back(std::move(item));
    }
}

void EditorSystem::PasteEntities(Scene* targetScene, EntityID parent)
{
    if (entityClipboard.empty()) return;

    Scene* scene = targetScene ? targetScene : GetTargetScene();
    if (!scene || !scene->IsEditable()) return;

    std::vector<EntityID> createdEntities;
    for (const auto& item : entityClipboard) {
        if (!item.doc) continue;

        EntityID targetParent = parent;
        if (targetParent == MAX_ENTITIES || targetParent == (EntityID)-1) {
            if (item.originalParent != MAX_ENTITIES && item.originalParent != (EntityID)-1) {
                if (scene->IsEntityActive(item.originalParent)) {
                    targetParent = item.originalParent;
                }
            }
        }

        Entity newEnt = scene->DeserializeObjectFromJson(*item.doc, targetParent);
        if (newEnt != MAX_ENTITIES) {
            createdEntities.push_back(newEnt.GetID());
        }
    }

    if (!createdEntities.empty()) {
        std::string srcName = (createdEntities.size() == 1) ? GetEntityRawName(createdEntities[0], scene) : (std::to_string(createdEntities.size()) + " Entities");
        auto cmd = std::make_shared<editor::InstantiateEntitiesCommand>(scene, createdEntities, "Paste " + srcName, this);
        history.Push(cmd);
        SetSelectedEntities(createdEntities);
        if (engine) {
            selectedScene = engine->GetScene(scene->GetName());
        }
    }
}

void EditorSystem::DuplicateSelectedEntities(Scene* targetScene)
{
    Scene* scene = targetScene ? targetScene : GetTargetScene();
    if (!scene || !scene->IsEditable() || selectedEntities.empty()) return;

    auto isDescendant = [scene](EntityID e, EntityID ancestor) -> bool {
        if (!scene || e == MAX_ENTITIES || ancestor == MAX_ENTITIES) return false;
        EntityID current = scene->GetParent(e);
        while (current != MAX_ENTITIES && current != (EntityID)-1) {
            if (current == ancestor) return true;
            current = scene->GetParent(current);
        }
        return false;
    };

    std::vector<EntityID> topLevelSelected;
    for (EntityID e : selectedEntities) {
        if (!scene->IsEntityActive(e) && !scene->HasComponent<TransformComponent>(e)) continue;
        bool hasSelectedAncestor = false;
        for (EntityID other : selectedEntities) {
            if (other != e && isDescendant(e, other)) {
                hasSelectedAncestor = true;
                break;
            }
        }
        if (!hasSelectedAncestor) {
            if (std::find(topLevelSelected.begin(), topLevelSelected.end(), e) == topLevelSelected.end()) {
                topLevelSelected.push_back(e);
            }
        }
    }

    if (topLevelSelected.empty()) return;

    std::vector<EntityID> duplicatedEntities;
    for (EntityID e : topLevelSelected) {
        rapidjson::Document doc;
        scene->SerializeObjectToJson(e, doc);

        EntityID parent = scene->GetParent(e);
        Entity newEnt = scene->DeserializeObjectFromJson(doc, parent);
        if (newEnt != MAX_ENTITIES) {
            duplicatedEntities.push_back(newEnt.GetID());
        }
    }

    if (!duplicatedEntities.empty()) {
        std::string srcName = (duplicatedEntities.size() == 1) ? GetEntityRawName(duplicatedEntities[0], scene) : (std::to_string(duplicatedEntities.size()) + " Entities");
        auto cmd = std::make_shared<editor::InstantiateEntitiesCommand>(scene, duplicatedEntities, "Duplicate " + srcName, this);
        history.Push(cmd);
        SetSelectedEntities(duplicatedEntities);
        if (engine) {
            selectedScene = engine->GetScene(scene->GetName());
        }
    }
}

void EditorSystem::Initialize()
{
    InitializeScriptRegistry();

    ForEachType<EngineComponents>([this]<typename T>() {
        this->RegisterComponentType<T>();
    });

    SetUpCameraControls();

    if (engine && engine->assetManagerInterface && engine->graphicsEngine) {
        auto wiremeshOpt = engine->assetManagerInterface->getAssetUuid("wiremeshShader");
        if (wiremeshOpt) {
            wiremeshShaderId = wiremeshOpt.value();
            wiremeshShaderHandle = engine->graphicsEngine->loadShader(wiremeshShaderId);
        }

        auto wiremeshTexOpt = engine->assetManagerInterface->getAssetUuid("wiremeshTexturedShader");
        if (wiremeshTexOpt) {
            wiremeshTexturedShaderId = wiremeshTexOpt.value();
            wiremeshTexturedShaderHandle = engine->graphicsEngine->loadShader(wiremeshTexturedShaderId);
        }
    }
}

void EditorSystem::SetUpCameraControls()
{
    // Initialize camera position
    UpdateCameraPosition();
}

void EditorSystem::EditorCameraState::UpdateCameraPosition()
{
    float yawRad = glm::radians(cameraYaw);
    float pitchRad = glm::radians(cameraPitch);
    cameraTransform.position.x = cameraTarget.x + cameraDistance * cos(pitchRad) * sin(yawRad);
    cameraTransform.position.y = cameraTarget.y + cameraDistance * sin(pitchRad);
    cameraTransform.position.z = cameraTarget.z + cameraDistance * cos(pitchRad) * cos(yawRad);
    glm::vec3 direction = glm::normalize(cameraTarget - cameraTransform.position);
    cameraTransform.rotation = glm::quatLookAt(direction, glm::vec3(0.0f, 1.0f, 0.0f));

    computeLocalMatrix(cameraTransform);
    cameraTransform.globalMatrix = cameraTransform.localMatrix;
    updateViewMatrix(camera, cameraTransform.globalMatrix);
}

EditorSystem::EditorCameraState& EditorSystem::GetEditorCameraState(Scene* scene)
{
    std::string name = scene ? scene->GetName() : "";
    return GetEditorCameraState(name);
}

EditorSystem::EditorCameraState& EditorSystem::GetEditorCameraState(const std::string& sceneName)
{
    auto it = sceneEditorCameras.find(sceneName);
    if (it == sceneEditorCameras.end()) {
        EditorCameraState state;
        state.cameraDistance = 5.0f;
        state.cameraYaw = 0.0f;
        state.cameraPitch = 45.0f;
        state.cameraTarget = glm::vec3(0.0f);
        state.camera.skyboxType = CameraComponent::SkyboxType::Color;
        state.camera.skyboxData = SkyboxColorData{glm::vec4(0.2f, 0.2f, 0.2f, 1.0f)};
        state.UpdateCameraPosition();
        sceneEditorCameras[sceneName] = state;
        return sceneEditorCameras[sceneName];
    }
    return it->second;
}

void EditorSystem::FocusCameraOnBounds(const glm::vec3& center, float distance, Scene* scene)
{
    Scene* targetScene = scene ? scene : GetTargetScene();
    std::string sName = targetScene ? targetScene->GetName() : "";
    auto& camState = GetEditorCameraState(sName);
    camState.cameraTarget = center;
    camState.cameraDistance = std::max(0.5f, distance);
    camState.cameraPitch = 30.0f;
    camState.cameraYaw = 45.0f;
    camState.UpdateCameraPosition();

    camera = camState.camera;
    cameraTransform = camState.cameraTransform;
    cameraDistance = camState.cameraDistance;
    cameraPitch = camState.cameraPitch;
    cameraYaw = camState.cameraYaw;
    cameraTarget = camState.cameraTarget;
}

void EditorSystem::UpdateCameraPosition()
{
    Scene* targetScene = GetTargetScene();
    std::string sName = targetScene ? targetScene->GetName() : "";
    auto& camState = GetEditorCameraState(sName);
    camState.cameraDistance = cameraDistance;
    camState.cameraYaw = cameraYaw;
    camState.cameraPitch = cameraPitch;
    camState.cameraTarget = cameraTarget;
    camState.UpdateCameraPosition();

    camera = camState.camera;
    cameraTransform = camState.cameraTransform;
}

void EditorSystem::ImGuiSceneGraph()
{
    sceneGraphNodeRects.clear();
    anySceneGraphNodeClickedOrActive = false;

    ImGui::Begin("Scene graph", nullptr, ImGuiWindowFlags_MenuBar);

    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 scroll = ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY());
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImGuiIO& io = ImGui::GetIO();
    bool isWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
    bool isCtrlOrCmd = io.KeyCtrl || io.KeySuper;
    bool isShift = io.KeyShift;

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::Button("New Scene"))
        {
            if (engine)
            {
                std::string baseName = "New Scene";
                std::string sceneName = baseName;
                int counter = 1;
                while (engine->GetScenes().find(sceneName) != engine->GetScenes().end())
                {
                    sceneName = baseName + " " + std::to_string(counter++);
                }
                auto newScene = engine->CreateScene(sceneName);
                if (newScene)
                {
                    newScene->SetActive(true);
                    selectedScene = newScene;
                    ClearSelectedEntities();
                }
            }
        }
        if (ImGui::Button("Save Scene"))
        {
            if (engine) engine->SaveScene();
        }
        ImGui::EndMenuBar();
    }

    if (!engine)
    {
        ImGui::End();
        return;
    }

    Scene* targetScene = GetTargetScene();

    std::string sceneToClose;
    for (const auto& sceneName : engine->GetSceneOrder())
    {
        auto scn = engine->GetScene(sceneName);
        if (!scn || scn->GetSceneType() == ecs::SceneType::Thumbnail) continue;
        ImGui::PushID(sceneName.c_str());
        bool isCurrentActive = scn->IsActive();
        if (ImGui::Checkbox("##active", &isCurrentActive))
        {
            scn->SetActive(isCurrentActive);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle Scene Active");
        if (ImGui::IsItemHovered() && (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right))) {
            anySceneGraphNodeClickedOrActive = true;
        }
        if (ImGui::IsItemActive()) {
            anySceneGraphNodeClickedOrActive = true;
        }
        ImGui::SameLine();

        ImGuiTreeNodeFlags sceneFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen;
        if (scn.get() == targetScene)
        {
            sceneFlags |= ImGuiTreeNodeFlags_Selected;
        }

        std::string displayName = sceneName + (scn->IsEditable() ? "" : " (Read-Only)");
        bool sceneNodeOpen = ImGui::TreeNodeEx((void*)scn.get(), sceneFlags, "%s", displayName.c_str());
        if (ImGui::IsItemHovered() && (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right))) {
            anySceneGraphNodeClickedOrActive = true;
        }
        if (ImGui::IsItemActive()) {
            anySceneGraphNodeClickedOrActive = true;
        }
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        {
            selectedScene = scn;
        }

        if (scn->IsEditable() && ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY"))
            {
                auto ents = ParseSceneEntityPayload(payload);
                for (EntityID droppedEntity : ents) {
                    EntityID oldP = scn->GetParent(droppedEntity);
                    if (oldP != MAX_ENTITIES && oldP != (EntityID)-1) {
                        auto cmd = std::make_shared<editor::ReparentEntityCommand>(scn.get(), droppedEntity, oldP, MAX_ENTITIES);
                        history.Execute(cmd);
                    }
                }
            }
            else if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AM_FILE_PATH"))
            {
                auto droppedPaths = ParseFilePathPayload(payload);
                std::vector<EntityID> createdEntities;
                for (const auto& p : droppedPaths)
                {
                    auto ext = p.extension().string();
                    if (ext == ".scene") {
                        auto uuid = engine->assetManagerInterface->getAssetUuidByPath(p);
                        if (uuid) engine->LoadScene(uuid.value());
                    } else if (ext == ".prefab") {
                        auto uuid = engine->assetManagerInterface->getAssetUuidByPath(p);
                        if (!uuid) uuid = engine->assetManagerInterface->registerAsset(p.string());
                        if (uuid) {
                            Entity newEnt = scn->InstantiatePrefab(uuid.value());
                            if (newEnt != MAX_ENTITIES) {
                                createdEntities.push_back(newEnt.GetID());
                            }
                        }
                    } else if (ext == ".fbx" || ext == ".obj" || ext == ".model" || ext == ".mesh" || ext == ".gltf" || ext == ".glb" || ext == ".blend" || ext == ".blend1") {
                        auto uuid = engine->assetManagerInterface->getAssetUuidByPath(p);
                        if (!uuid) uuid = engine->assetManagerInterface->registerAsset(p.string());
                        if (uuid) {
                            Entity newEnt = scn->InstantiateModel(uuid.value());
                            if (newEnt != MAX_ENTITIES) {
                                createdEntities.push_back(newEnt.GetID());
                            }
                        }
                    }
                }
                if (!createdEntities.empty()) {
                    auto cmd = std::make_shared<editor::InstantiateEntitiesCommand>(scn.get(), createdEntities, "Dropped Assets", this);
                    history.Push(cmd);
                    SetSelectedEntities(createdEntities);
                    selectedScene = scn;
                }
            }
            ImGui::EndDragDropTarget();
        }

        // Context menu for the scene
        if (ImGui::BeginPopupContextItem())
        {
            if (scn->IsEditable())
            {
                if (ImGui::MenuItem("Create Entity"))
                {
                    selectedScene = scn;
                    Entity newEnt = scn->CreateEntity();
                    auto cmd = std::make_shared<editor::CreateEntityCommand>(scn.get(), newEnt, MAX_ENTITIES, GetEntityRawName(newEnt, scn.get()));
                    history.Push(cmd);
                }
                if (HasEntitiesInClipboard())
                {
                    if (ImGui::MenuItem("Paste Entities", "Ctrl+V"))
                    {
                        selectedScene = scn;
                        PasteEntities(scn.get());
                    }
                }
            }
            if (ImGui::MenuItem(isCurrentActive ? "Deactivate Scene" : "Activate Scene"))
            {
                scn->SetActive(!isCurrentActive);
            }
            if (ImGui::MenuItem("Close Scene"))
            {
                sceneToClose = sceneName;
            }
            ImGui::EndPopup();
        }

        if (sceneNodeOpen)
        {
            auto rootsCopy = scn->GetRootEntities();
            for (Entity root : rootsCopy)
            {
                ImGuiGraphEntity(scn.get(), root);
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    if (!sceneToClose.empty())
    {
        if (auto currentSelected = selectedScene.lock())
        {
            if (currentSelected->GetName() == sceneToClose)
            {
                selectedScene.reset();
                ClearSelectedEntities();
                renamingEntity = std::numeric_limits<std::uint32_t>::max();
            }
        }
        engine->CloseScene(sceneToClose);
    }

    if (targetScene && targetScene->IsEditable() && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !selectedEntities.empty() && renamingEntity == std::numeric_limits<std::uint32_t>::max())
    {
        if (ImGui::IsKeyPressed(ImGuiKey_Delete))
        {
            auto toDel = selectedEntities;
            auto cmd = std::make_shared<editor::DeleteEntitiesCommand>(targetScene, toDel, this);
            history.Execute(cmd);
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_F2))
        {
            Entity primary = GetSelectedEntity();
            if (primary != std::numeric_limits<std::uint32_t>::max()) {
                renamingEntity = primary;
                std::snprintf(renameBuf, sizeof(renameBuf), "%s", GetEntityRawName(primary, targetScene).c_str());
                renameFocusRequested = true;
            }
        }
    }

    // Handle dropping onto empty space (to make an entity a root or instantiate an asset)
    ImVec2 availSpace = ImGui::GetContentRegionAvail();
    if (availSpace.y > 10.0f)
    {
        ImVec2 emptyMin = ImGui::GetCursorScreenPos();
        ImGui::Dummy(availSpace);
        ImVec2 emptyMax = ImGui::GetItemRectMax();
        bool isEmptySceneDropTarget = false;
        if (targetScene && targetScene->IsEditable() && ImGui::BeginDragDropTarget())
        {
            isEmptySceneDropTarget = true;
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY"))
            {
                auto ents = ParseSceneEntityPayload(payload);
                for (EntityID droppedEntity : ents) {
                    EntityID oldP = targetScene->GetParent(droppedEntity);
                    if (oldP != MAX_ENTITIES && oldP != (EntityID)-1) {
                        auto cmd = std::make_shared<editor::ReparentEntityCommand>(targetScene, droppedEntity, oldP, MAX_ENTITIES);
                        history.Execute(cmd);
                    }
                }
            }
            else if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AM_FILE_PATH"))
            {
                auto droppedPaths = ParseFilePathPayload(payload);
                std::vector<EntityID> createdEntities;
                for (const auto& p : droppedPaths)
                {
                    auto ext = p.extension().string();
                    if (ext == ".scene") {
                        auto uuid = engine->assetManagerInterface->getAssetUuidByPath(p);
                        if (uuid) engine->LoadScene(uuid.value());
                    } else if (ext == ".prefab") {
                        auto uuid = engine->assetManagerInterface->getAssetUuidByPath(p);
                        if (!uuid) uuid = engine->assetManagerInterface->registerAsset(p.string());
                        if (uuid) {
                            Entity newEnt = targetScene->InstantiatePrefab(uuid.value());
                            if (newEnt != MAX_ENTITIES) {
                                createdEntities.push_back(newEnt.GetID());
                            }
                        }
                    } else if (ext == ".fbx" || ext == ".obj" || ext == ".model" || ext == ".mesh" || ext == ".gltf" || ext == ".glb" || ext == ".blend" || ext == ".blend1") {
                        auto uuid = engine->assetManagerInterface->getAssetUuidByPath(p);
                        if (!uuid) uuid = engine->assetManagerInterface->registerAsset(p.string());
                        if (uuid) {
                            Entity newEnt = targetScene->InstantiateModel(uuid.value());
                            if (newEnt != MAX_ENTITIES) {
                                createdEntities.push_back(newEnt.GetID());
                            }
                        }
                    }
                }
                if (!createdEntities.empty()) {
                    auto cmd = std::make_shared<editor::InstantiateEntitiesCommand>(targetScene, createdEntities, "Dropped Assets", this);
                    history.Push(cmd);
                    SetSelectedEntities(createdEntities);
                    selectedScene = targetScene->engine.GetScene(targetScene->GetName());
                }
            }
            ImGui::EndDragDropTarget();
        }
        if (isEmptySceneDropTarget)
        {
            ImGui::GetWindowDrawList()->AddRect(emptyMin, emptyMax, IM_COL32(66, 180, 255, 180), 4.0f, 0, 2.0f);
            ImGui::GetWindowDrawList()->AddRectFilled(emptyMin, emptyMax, IM_COL32(66, 180, 255, 30), 4.0f);
        }
    }

    // Context menu on empty area
    if (targetScene && targetScene->IsEditable() && ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight))
    {
        if (!selectedEntities.empty())
        {
            std::string copyLabel = selectedEntities.size() > 1 ? ("Copy (" + std::to_string(selectedEntities.size()) + " entities)") : "Copy";
            if (ImGui::MenuItem(copyLabel.c_str(), "Ctrl+C"))
            {
                CopySelectedEntities(targetScene);
            }
            std::string dupLabel = selectedEntities.size() > 1 ? ("Duplicate (" + std::to_string(selectedEntities.size()) + " entities)") : "Duplicate";
            if (ImGui::MenuItem(dupLabel.c_str(), "Ctrl+D"))
            {
                DuplicateSelectedEntities(targetScene);
            }
            std::string delLabel = selectedEntities.size() > 1 ? ("Delete (" + std::to_string(selectedEntities.size()) + " entities)") : "Delete Entity";
            if (ImGui::MenuItem(delLabel.c_str(), "Del"))
            {
                auto toDel = selectedEntities;
                auto cmd = std::make_shared<editor::DeleteEntitiesCommand>(targetScene, toDel, this);
                history.Execute(cmd);
            }
            ImGui::Separator();
        }
        if (HasEntitiesInClipboard())
        {
            if (ImGui::MenuItem("Paste", "Ctrl+V"))
            {
                PasteEntities(targetScene);
            }
            ImGui::Separator();
        }
        if (ImGui::MenuItem("Create Entity"))
        {
            Entity newEnt = targetScene->CreateEntity();
            auto cmd = std::make_shared<editor::CreateEntityCommand>(targetScene, newEnt, MAX_ENTITIES, GetEntityRawName(newEnt, targetScene));
            history.Push(cmd);
        }
        ImGui::EndPopup();
    }

    // Box selection (Marquee selection)
    if (isWindowHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !anySceneGraphNodeClickedOrActive && !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId) && !ImGui::IsDragDropActive())
    {
        isSceneGraphBoxSelecting = true;
        sceneGraphBoxSelectStartPos = ImVec2(io.MousePos.x - windowPos.x + scroll.x, io.MousePos.y - windowPos.y + scroll.y);
        sceneGraphBoxSelectPreSelection = selectedEntities;
    }

    if (isSceneGraphBoxSelecting)
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            ImVec2 curContentPos = ImVec2(io.MousePos.x - windowPos.x + scroll.x, io.MousePos.y - windowPos.y + scroll.y);
            float dragDist = std::hypot(curContentPos.x - sceneGraphBoxSelectStartPos.x, curContentPos.y - sceneGraphBoxSelectStartPos.y);

            if (dragDist > 3.0f)
            {
                ImVec2 boxMinContent = ImVec2(std::min(sceneGraphBoxSelectStartPos.x, curContentPos.x), std::min(sceneGraphBoxSelectStartPos.y, curContentPos.y));
                ImVec2 boxMaxContent = ImVec2(std::max(sceneGraphBoxSelectStartPos.x, curContentPos.x), std::max(sceneGraphBoxSelectStartPos.y, curContentPos.y));

                ImVec2 boxMinScreen = ImVec2(boxMinContent.x + windowPos.x - scroll.x, boxMinContent.y + windowPos.y - scroll.y);
                ImVec2 boxMaxScreen = ImVec2(boxMaxContent.x + windowPos.x - scroll.x, boxMaxContent.y + windowPos.y - scroll.y);

                ImVec2 clipMin = windowPos;
                ImVec2 clipMax = ImVec2(windowPos.x + ImGui::GetWindowSize().x, windowPos.y + ImGui::GetWindowSize().y);
                ImVec2 drawBoxMin = ImVec2(std::max(boxMinScreen.x, clipMin.x), std::max(boxMinScreen.y, clipMin.y));
                ImVec2 drawBoxMax = ImVec2(std::min(boxMaxScreen.x, clipMax.x), std::min(boxMaxScreen.y, clipMax.y));

                if (drawBoxMax.x > drawBoxMin.x && drawBoxMax.y > drawBoxMin.y)
                {
                    drawList->AddRectFilled(drawBoxMin, drawBoxMax, IM_COL32(66, 150, 250, 45));
                    drawList->AddRect(drawBoxMin, drawBoxMax, IM_COL32(66, 150, 250, 220), 0.0f, 0, 1.5f);
                }

                std::vector<EntityID> inBoxEntities;
                Scene* primaryScene = nullptr;
                for (const auto& nr : sceneGraphNodeRects)
                {
                    bool overlaps = (boxMinContent.x <= nr.contentMax.x && boxMaxContent.x >= nr.contentMin.x &&
                                     boxMinContent.y <= nr.contentMax.y && boxMaxContent.y >= nr.contentMin.y);
                    if (overlaps)
                    {
                        inBoxEntities.push_back(nr.entity.GetID());
                        if (!primaryScene) primaryScene = nr.scene;
                    }
                }

                if (primaryScene) {
                    selectedScene = primaryScene->engine.GetScene(primaryScene->GetName());
                }

                if (isCtrlOrCmd)
                {
                    std::vector<EntityID> combined = sceneGraphBoxSelectPreSelection;
                    for (EntityID e : inBoxEntities)
                    {
                        auto it = std::find(combined.begin(), combined.end(), e);
                        if (it != combined.end())
                        {
                            combined.erase(it);
                        }
                        else
                        {
                            combined.push_back(e);
                        }
                    }
                    SetSelectedEntities(combined);
                }
                else if (isShift)
                {
                    std::vector<EntityID> combined = sceneGraphBoxSelectPreSelection;
                    for (EntityID e : inBoxEntities)
                    {
                        if (std::find(combined.begin(), combined.end(), e) == combined.end())
                        {
                            combined.push_back(e);
                        }
                    }
                    SetSelectedEntities(combined);
                }
                else
                {
                    SetSelectedEntities(inBoxEntities);
                }
            }
        }
        else
        {
            ImVec2 curContentPos = ImVec2(io.MousePos.x - windowPos.x + scroll.x, io.MousePos.y - windowPos.y + scroll.y);
            float dragDist = std::hypot(curContentPos.x - sceneGraphBoxSelectStartPos.x, curContentPos.y - sceneGraphBoxSelectStartPos.y);
            if (dragDist <= 3.0f)
            {
                if (!isCtrlOrCmd && !isShift)
                {
                    ClearSelectedEntities();
                }
            }
            isSceneGraphBoxSelecting = false;
        }
    }

    ImGui::End();
}

void EditorSystem::ImGuiGraphEntity(Scene* currentScene, Entity entity)
{
    if (!currentScene) return;
    std::string nameStr = GetEntityName(entity, currentScene);
    auto children = currentScene->GetChildren(entity);
    bool hasChildren = !children.empty();

    ImGuiTreeNodeFlags flags = hasChildren ? 0 : ImGuiTreeNodeFlags_Leaf;
    flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

    Scene* targetScene = GetTargetScene();
    if (IsEntitySelected(entity) && currentScene == targetScene) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool isActive = currentScene->IsEntityActive(entity);
    if (!isActive) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    }

    bool isRenaming = (renamingEntity == entity && currentScene == targetScene && currentScene->IsEditable());
    bool nodeOpen = false;

    if (isRenaming)
    {
        flags |= ImGuiTreeNodeFlags_AllowOverlap;
        nodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)entity, flags, "(#%u) ", entity);
        ImGui::SameLine();
        if (renameFocusRequested)
        {
            ImGui::SetKeyboardFocusHere();
            renameFocusRequested = false;
        }
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 10.0f);
        ImGuiInputTextFlags inputFlags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll;
        if (ImGui::InputText("##TreeRename", renameBuf, sizeof(renameBuf), inputFlags))
        {
            std::string oldName = GetEntityRawName(entity, currentScene);
            if (oldName != renameBuf) {
                auto cmd = std::make_shared<editor::RenameEntityCommand>(this, currentScene, entity, oldName, renameBuf);
                history.Execute(cmd);
            }
            renamingEntity = std::numeric_limits<std::uint32_t>::max();
        }
        else if (ImGui::IsItemDeactivated())
        {
            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                renamingEntity = std::numeric_limits<std::uint32_t>::max();
            }
            else
            {
                std::string oldName = GetEntityRawName(entity, currentScene);
                if (oldName != renameBuf) {
                    auto cmd = std::make_shared<editor::RenameEntityCommand>(this, currentScene, entity, oldName, renameBuf);
                    history.Execute(cmd);
                }
                renamingEntity = std::numeric_limits<std::uint32_t>::max();
            }
        }
    }
    else
    {
        nodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)entity, flags, "%s", nameStr.c_str());
    }

    if (!isActive) {
        ImGui::PopStyleColor();
    }

    ImVec2 itemMin = ImGui::GetItemRectMin();
    ImVec2 itemMax = ImGui::GetItemRectMax();
    bool isNodeHovered = ImGui::IsItemHovered();
    bool isNodeActive = ImGui::IsItemActive();

    if (isNodeHovered && (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right))) {
        anySceneGraphNodeClickedOrActive = true;
    }
    if (isNodeActive) {
        anySceneGraphNodeClickedOrActive = true;
    }

    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 scroll = ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY());
    float rowMinX = windowPos.x;
    float rowMaxX = windowPos.x + ImGui::GetWindowContentRegionMax().x;
    float rowMinY = itemMin.y;
    float rowMaxY = itemMax.y;

    ImVec2 rowContentMin = ImVec2(rowMinX - windowPos.x + scroll.x, rowMinY - windowPos.y + scroll.y);
    ImVec2 rowContentMax = ImVec2(rowMaxX - windowPos.x + scroll.x, rowMaxY - windowPos.y + scroll.y);
    sceneGraphNodeRects.push_back({ entity, currentScene, rowContentMin, rowContentMax });

    // Context menu on entity
    if (ImGui::BeginPopupContextItem())
    {
        if (!IsEntitySelected(entity)) {
            SetSelectedEntity(entity);
        }

        if (currentScene->IsEditable())
        {
            if (selectedEntities.size() > 1 && IsEntitySelected(entity))
            {
                std::string copyLabel = "Copy (" + std::to_string(selectedEntities.size()) + " entities)";
                std::string dupLabel = "Duplicate (" + std::to_string(selectedEntities.size()) + " entities)";
                std::string delLabel = "Delete (" + std::to_string(selectedEntities.size()) + " entities)";
                std::string saveLabel = "Save (" + std::to_string(selectedEntities.size()) + " entities) as Prefabs";
                if (ImGui::MenuItem(copyLabel.c_str(), "Ctrl+C"))
                {
                    CopySelectedEntities(currentScene);
                }
                if (ImGui::MenuItem(dupLabel.c_str(), "Ctrl+D"))
                {
                    DuplicateSelectedEntities(currentScene);
                }
                if (HasEntitiesInClipboard())
                {
                    if (ImGui::MenuItem("Paste", "Ctrl+V"))
                    {
                        PasteEntities(currentScene);
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem(saveLabel.c_str()))
                {
                    for (Entity e : selectedEntities) {
                        SaveEntityAsPrefab(e, "res/prefabs", currentScene);
                    }
                }
                if (ImGui::MenuItem("Enable All Selected"))
                {
                    std::vector<std::pair<Entity, bool>> oldStates;
                    for (Entity e : selectedEntities) {
                        oldStates.push_back({e, currentScene->IsEntityActive(e)});
                    }
                    auto cmd = std::make_shared<editor::SetEntityActiveCommand>(currentScene, oldStates, true);
                    history.Execute(cmd);
                }
                if (ImGui::MenuItem("Disable All Selected"))
                {
                    std::vector<std::pair<Entity, bool>> oldStates;
                    for (Entity e : selectedEntities) {
                        oldStates.push_back({e, currentScene->IsEntityActive(e)});
                    }
                    auto cmd = std::make_shared<editor::SetEntityActiveCommand>(currentScene, oldStates, false);
                    history.Execute(cmd);
                }
                ImGui::Separator();
                if (ImGui::MenuItem(delLabel.c_str()))
                {
                    auto toDel = selectedEntities;
                    auto cmd = std::make_shared<editor::DeleteEntitiesCommand>(currentScene, toDel, this);
                    history.Execute(cmd);
                    renamingEntity = std::numeric_limits<std::uint32_t>::max();
                    ImGui::EndPopup();
                    if (nodeOpen) ImGui::TreePop();
                    return;
                }
            }
            else
            {
                if (ImGui::MenuItem("Copy", "Ctrl+C"))
                {
                    CopySelectedEntities(currentScene);
                }
                if (ImGui::MenuItem("Duplicate", "Ctrl+D"))
                {
                    DuplicateSelectedEntities(currentScene);
                }
                if (HasEntitiesInClipboard())
                {
                    if (ImGui::MenuItem("Paste", "Ctrl+V"))
                    {
                        PasteEntities(currentScene);
                    }
                    if (ImGui::MenuItem("Paste as Child"))
                    {
                        PasteEntities(currentScene, entity);
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Rename", "F2"))
                {
                    renamingEntity = entity;
                    std::snprintf(renameBuf, sizeof(renameBuf), "%s", GetEntityRawName(entity, currentScene).c_str());
                    renameFocusRequested = true;
                }
                if (ImGui::MenuItem("Create Child Entity"))
                {
                    Entity childEnt = currentScene->CreateEntity("Child Entity", entity);
                    auto cmd = std::make_shared<editor::CreateEntityCommand>(currentScene, childEnt, entity, "Child Entity");
                    history.Push(cmd);
                }
                if (ImGui::MenuItem(isActive ? "Disable Entity" : "Enable Entity"))
                {
                    auto cmd = std::make_shared<editor::SetEntityActiveCommand>(currentScene, std::vector<std::pair<Entity, bool>>{{entity, isActive}}, !isActive);
                    history.Execute(cmd);
                }
                if (ImGui::MenuItem("Save as Prefab"))
                {
                    SaveEntityAsPrefab(entity, "res/prefabs", currentScene);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Delete Entity"))
                {
                    auto cmd = std::make_shared<editor::DeleteEntitiesCommand>(currentScene, std::vector<EntityID>{entity.GetID()}, this);
                    history.Execute(cmd);
                    if (renamingEntity == entity)
                    {
                        renamingEntity = std::numeric_limits<std::uint32_t>::max();
                    }
                    ImGui::EndPopup();
                    if (nodeOpen)
                    {
                        ImGui::TreePop();
                    }
                    return;
                }
            }
        }
        else
        {
            ImGui::TextDisabled("Scene is Read-Only");
        }
        ImGui::EndPopup();
    }

    // Start drag operation
    if (currentScene->IsEditable() && !isRenaming && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
    {
        std::vector<EntityID> entsToDrag;
        if (IsEntitySelected(entity.GetID()) && selectedEntities.size() > 1) {
            entsToDrag = selectedEntities;
        } else {
            entsToDrag.push_back(entity.GetID());
        }

        engine::ecs::SceneEntityPayload payloadData{};
        payloadData.entity = entity.GetID();
        payloadData.scene = currentScene;
        payloadData.count = (uint32_t)std::min(entsToDrag.size(), (size_t)128);
        for (uint32_t k = 0; k < payloadData.count; ++k) {
            payloadData.entities[k] = entsToDrag[k];
        }

        ImGui::SetDragDropPayload("SCENE_ENTITY", &payloadData, sizeof(engine::ecs::SceneEntityPayload));
        ImGui::BeginGroup();
        ImFont* iconFont = (ImGui::GetIO().Fonts->Fonts.Size > 1) ? ImGui::GetIO().Fonts->Fonts[1] : ImGui::GetFont();
        ImGui::PushFont(iconFont);
        ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.0f, 1.0f), "%s", payloadData.count > 1 ? ICON_FA_BOXES_STACKED : ICON_FA_CUBE);
        ImGui::PopFont();
        ImGui::SameLine();
        ImGui::BeginGroup();
        if (payloadData.count > 1) {
            ImGui::Text("%u Entities", payloadData.count);
            ImGui::TextDisabled("Drag to reparent or save as prefabs");
        } else {
            ImGui::TextUnformatted(nameStr.c_str());
            ImGui::TextDisabled("Entity #%u - Drag to reparent or save as prefab", entity);
        }
        ImGui::EndGroup();
        ImGui::EndGroup();
        ImGui::EndDragDropSource();
    }

    // Drag highlight if this node is among the entities currently being dragged
    const ImGuiPayload* curDragPayload = ImGui::GetDragDropPayload();
    if (curDragPayload && curDragPayload->IsDataType("SCENE_ENTITY") && curDragPayload->Data) {
        auto draggedEnts = ParseSceneEntityPayload(curDragPayload);
        for (EntityID de : draggedEnts) {
            if (de == entity.GetID()) {
                ImVec2 itemMin = ImGui::GetItemRectMin();
                ImVec2 itemMax = ImGui::GetItemRectMax();
                ImGui::GetWindowDrawList()->AddRect(itemMin, itemMax, IM_COL32(66, 180, 255, 120), 0.0f, 0, 1.0f);
                ImGui::GetWindowDrawList()->AddRectFilled(itemMin, itemMax, IM_COL32(66, 180, 255, 30));
                break;
            }
        }
    }

    // Handle incoming drag
    bool isNodeDropTarget = false;
    if (currentScene->IsEditable() && ImGui::BeginDragDropTarget())
    {
        isNodeDropTarget = true;
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY"))
        {
            auto ents = ParseSceneEntityPayload(payload);
            for (EntityID droppedEntity : ents) {
                // Prevent dropping on itself
                if (droppedEntity != entity.GetID())
                {
                    EntityID oldP = currentScene->GetParent(droppedEntity);
                    if (oldP != entity.GetID()) {
                        auto cmd = std::make_shared<editor::ReparentEntityCommand>(currentScene, droppedEntity, oldP, entity.GetID());
                        history.Execute(cmd);
                    }
                }
            }
        }
        else if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AM_FILE_PATH"))
        {
            auto droppedPaths = ParseFilePathPayload(payload);
            std::vector<EntityID> createdEntities;
            for (const auto& p : droppedPaths)
            {
                auto ext = p.extension().string();
                if (ext == ".scene") {
                    auto uuid = engine->assetManagerInterface->getAssetUuidByPath(p);
                    if (uuid) engine->LoadScene(uuid.value());
                } else if (ext == ".prefab") {
                    auto uuid = engine->assetManagerInterface->getAssetUuidByPath(p);
                    if (!uuid) uuid = engine->assetManagerInterface->registerAsset(p.string());
                    if (uuid) {
                        Entity newEnt = currentScene->InstantiatePrefab(uuid.value(), entity);
                        if (newEnt != MAX_ENTITIES) {
                            createdEntities.push_back(newEnt.GetID());
                        }
                    }
                } else if (ext == ".fbx" || ext == ".obj" || ext == ".model" || ext == ".mesh" || ext == ".gltf" || ext == ".glb" || ext == ".blend" || ext == ".blend1") {
                    auto uuid = engine->assetManagerInterface->getAssetUuidByPath(p);
                    if (!uuid) uuid = engine->assetManagerInterface->registerAsset(p.string());
                    if (uuid) {
                        Entity newEnt = currentScene->InstantiateModel(uuid.value(), entity);
                        if (newEnt != MAX_ENTITIES) {
                            createdEntities.push_back(newEnt.GetID());
                        }
                    }
                }
            }
            if (!createdEntities.empty()) {
                auto cmd = std::make_shared<editor::InstantiateEntitiesCommand>(currentScene, createdEntities, "Dropped Assets", this);
                history.Push(cmd);
                SetSelectedEntities(createdEntities);
                selectedScene = currentScene->engine.GetScene(currentScene->GetName());
            }
        }
        ImGui::EndDragDropTarget();
    }
    if (isNodeDropTarget)
    {
        ImVec2 itemMin = ImGui::GetItemRectMin();
        ImVec2 itemMax = ImGui::GetItemRectMax();
        ImGui::GetWindowDrawList()->AddRect(itemMin, itemMax, IM_COL32(66, 180, 255, 255), 2.0f, 0, 1.5f);
        ImGui::GetWindowDrawList()->AddRectFilled(itemMin, itemMax, IM_COL32(66, 180, 255, 45), 2.0f);
    }

    // Handle selection when clicked
    if (!isRenaming && !isSceneGraphBoxSelecting) {
        bool isCtrl = ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeySuper;
        bool isShift = ImGui::GetIO().KeyShift;

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            selectedScene = currentScene->engine.GetScene(currentScene->GetName());
            if (isCtrl) {
                if (IsEntitySelected(entity)) {
                    RemoveSelectedEntity(entity);
                } else {
                    AddSelectedEntity(entity);
                }
            } else if (isShift) {
                AddSelectedEntity(entity);
            } else {
                if (!IsEntitySelected(entity)) {
                    SetSelectedEntity(entity);
                }
            }
        } else if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left) && !isCtrl && !isShift && !ImGui::IsMouseDragging(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen()) {
            if (IsEntitySelected(entity) && selectedEntities.size() > 1) {
                SetSelectedEntity(entity);
            }
        }
    }

    if (nodeOpen) {
        ImGui::Indent();
        auto childrenCopy = currentScene->GetChildren(entity);
        for (Entity child : childrenCopy) {
            ImGuiGraphEntity(currentScene, child);
        }
        ImGui::Unindent();
        ImGui::TreePop();
    }
}

void EditorSystem::ImGuiHistoryWindow(const std::string& windowName)
{
    if (!ImGui::Begin(windowName.c_str()))
    {
        ImGui::End();
        return;
    }

    const auto& undoStack = history.GetUndoStack();
    const auto& redoStack = history.GetRedoStack();

    // Top action bar
    bool canUndo = CanUndo();
    ImGui::BeginDisabled(!canUndo);
    if (ImGui::Button(ICON_FA_ROTATE_LEFT " Undo (Ctrl+Z)")) {
        Undo();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    bool canRedo = CanRedo();
    ImGui::BeginDisabled(!canRedo);
    if (ImGui::Button(ICON_FA_ROTATE_RIGHT " Redo (Ctrl+Shift+Z)")) {
        Redo();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    bool canClear = !undoStack.empty() || !redoStack.empty();
    ImGui::BeginDisabled(!canClear);
    if (ImGui::Button("Clear")) {
        history.Clear();
    }
    ImGui::EndDisabled();

    ImGui::Separator();

    ImGui::TextDisabled("History Steps: %zu undo, %zu redo", undoStack.size(), redoStack.size());
    ImGui::Separator();

    if (undoStack.empty() && redoStack.empty())
    {
        ImGui::TextDisabled("No history available");
        ImGui::End();
        return;
    }

    if (ImGui::BeginChild("HistoryList"))
    {
        // 1. Initial / Base state item
        bool isInitialActive = undoStack.empty();
        if (ImGui::Selectable("  [Initial State]", isInitialActive))
        {
            history.JumpToIndex(0);
        }

        // 2. Undo stack (past actions up to current)
        for (size_t i = 0; i < undoStack.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));
            bool isCurrent = (i == undoStack.size() - 1);
            std::string label = std::string(isCurrent ? ICON_FA_CHECK " " : "  ") + std::to_string(i + 1) + ". " + undoStack[i]->GetName();
            if (ImGui::Selectable(label.c_str(), isCurrent))
            {
                history.JumpToIndex(i + 1);
            }
            ImGui::PopID();
        }

        // 3. Redo stack (undone actions shown disabled)
        if (!redoStack.empty())
        {
            ImGui::Separator();
            ImGui::TextDisabled("-- Redo Available --");
            for (size_t i = 0; i < redoStack.size(); ++i)
            {
                size_t redoIdx = redoStack.size() - 1 - i;
                ImGui::PushID(static_cast<int>(10000 + i));
                std::string label = "  " + std::to_string(undoStack.size() + i + 1) + ". " + redoStack[redoIdx]->GetName();
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                if (ImGui::Selectable(label.c_str(), false))
                {
                    history.JumpToIndex(undoStack.size() + i + 1);
                }
                ImGui::PopStyleColor();
                ImGui::PopID();
            }
        }
    }
    ImGui::EndChild();

    ImGui::End();
}

void* EditorSystem::GetThumbnailTexture(const boost::uuids::uuid& assetId, const std::string& thumbnailPath)
{
    if (engine && engine->graphicsEngine) {
        return engine->graphicsEngine->getThumbnailTexture(assetId, thumbnailPath);
    }
    return nullptr;
}

void EditorSystem::InvalidateThumbnail(const boost::uuids::uuid& assetId)
{
    if (engine && engine->graphicsEngine) {
        engine->graphicsEngine->invalidateThumbnail(assetId);
    }
}

bool EditorSystem::QueueThumbnailGeneration(const boost::uuids::uuid& assetId, const std::string& outputPath)
{
    if (!engine) return false;
    std::string sceneName = "__thumbnail_" + boost::uuids::to_string(assetId);
    auto existingScene = engine->GetScene(sceneName);
    if (existingScene) {
        return true;
    }

    auto scene = engine->CreateScene(sceneName);
    scene->SetSceneType(ecs::SceneType::Thumbnail);
    scene->SetActive(true);

    if (!engine->SetupAssetInScene(scene.get(), assetId)) {
        engine->RemoveScene(sceneName);
        return false;
    }

    ecs::Entity thumbEntity = scene->CreateEntity("ThumbnailCapture");
    ecs::ThumbnailComponent thumbComp(assetId, outputPath);
    scene->AddComponent<ecs::ThumbnailComponent>(thumbEntity, thumbComp);

    return true;
}

std::shared_ptr<ecs::Scene> EditorSystem::OpenAssetPreviewScene(const boost::uuids::uuid& assetId)
{
    if (!engine) return nullptr;
    std::string sceneName = "Preview_" + boost::uuids::to_string(assetId);
    auto existingScene = engine->GetScene(sceneName);
    if (existingScene) {
        existingScene->SetActive(true);
        SetTargetScene(existingScene);
        auto roots = existingScene->GetRootEntities();
        if (!roots.empty()) {
            SetSelectedEntity(roots[0]);
        }
        return existingScene;
    }

    auto scene = engine->CreateScene(sceneName);
    scene->SetSceneType(ecs::SceneType::Preview);
    scene->SetActive(true);

    if (!engine->SetupAssetInScene(scene.get(), assetId)) {
        engine->RemoveScene(sceneName);
        return nullptr;
    }

    SetTargetScene(scene);
    auto roots = scene->GetRootEntities();
    if (!roots.empty()) {
        SetSelectedEntity(roots[0]);
    }
    auto cameraObj = scene->GetActiveCamera();
    if (cameraObj.transform) {
        cameraTransform = *cameraObj.transform;
    }
    if (cameraObj.camera) {
        camera = *cameraObj.camera;
    }

    return scene;
}

bool EditorSystem::SaveEntityAsPrefab(EntityID entity, const std::filesystem::path& destinationDirectoryOrPath, ecs::Scene* sourceScene)
{
    if (!engine || !engine->assetManagerInterface)
    {
        spdlog::error("SaveEntityAsPrefab failed: assetManagerInterface is null");
        return false;
    }

    auto assetManagerInterface = engine->assetManagerInterface;

    Scene* scene = sourceScene;
    if (!scene)
    {
        scene = GetTargetScene();
        if (!scene)
        {
            scene = engine->GetTopEditableScene().get();
        }
        if (!scene)
        {
            auto activeScenes = engine->GetActiveScenes();
            if (!activeScenes.empty())
            {
                scene = activeScenes[0].get();
            }
        }
    }

    if (!scene)
    {
        spdlog::error("SaveEntityAsPrefab failed: No valid scene found for entity {}", entity);
        return false;
    }

    std::string entityRawName = GetEntityRawName(entity, scene);
    if (entityRawName.empty())
    {
        entityRawName = "Entity";
    }

    std::filesystem::path targetPath = destinationDirectoryOrPath;
    std::error_code ec;
    if (std::filesystem::is_directory(targetPath, ec) || targetPath.extension() != ".prefab")
    {
        targetPath = targetPath / (entityRawName + ".prefab");
    }

    if (assetManagerInterface && assetManagerInterface->imguiFileBrowser)
    {
        targetPath = assetManagerInterface->imguiFileBrowser->getUniqueCopyPath(targetPath);
    }

    rapidjson::Document doc;
    scene->SerializeObjectToJson(entity, doc);

    auto pOpt = assetManagerInterface->createAsset(am::AssetType::Prefab, targetPath.string());
    if (!pOpt.has_value() || pOpt.value().is_nil())
    {
        spdlog::error("SaveEntityAsPrefab failed: Could not create prefab asset at {}", targetPath.string());
        return false;
    }

    auto info = assetManagerInterface->getAssetInfo(pOpt.value());
    if (info)
    {
        auto prefabAsset = dynamic_cast<am::PrefabAsset*>(info->get()->getAsset());
        if (prefabAsset)
        {
            prefabAsset->prefabData.CopyFrom(doc, prefabAsset->prefabData.GetAllocator());
            rapidjson::Document saveDoc;
            saveDoc.CopyFrom(doc, saveDoc.GetAllocator());
            if (!saveDoc.HasMember("uuid"))
            {
                rapidjson::Value uuidVal(boost::uuids::to_string(pOpt.value()).c_str(), saveDoc.GetAllocator());
                saveDoc.AddMember("uuid", uuidVal, saveDoc.GetAllocator());
            }
            am::saveJsonToFile(info->get()->path, saveDoc);
        }
        if (assetManagerInterface->imguiFileBrowser)
        {
            assetManagerInterface->imguiFileBrowser->setSelectedFile(info->get()->path);
        }
        spdlog::info("Saved entity {} ('{}') as prefab to '{}'", entity, entityRawName, info->get()->path);
    }
    else
    {
        if (assetManagerInterface->imguiFileBrowser)
        {
            assetManagerInterface->imguiFileBrowser->setSelectedFile(targetPath);
        }
        spdlog::info("Saved entity {} ('{}') as prefab to '{}'", entity, entityRawName, targetPath.string());
    }

    return true;
}

bool EditorSystem::CreateScriptAsset(const std::string& scriptName, const std::filesystem::path& targetDir)
{
    std::filesystem::path scriptDir = targetDir.empty() ? (std::filesystem::path("res") / "scripts") : targetDir;
    std::error_code ec;
    std::filesystem::create_directories(scriptDir, ec);

    std::filesystem::path filePath = scriptDir / (scriptName + ".hpp");
    if (std::filesystem::exists(filePath, ec)) {
        spdlog::warn("Script {} already exists at {}", scriptName, filePath.string());
        return false;
    }

    std::string sanitizedClassName;
    for (char c : scriptName) {
        if (isalnum(static_cast<unsigned char>(c)) || c == '_') {
            sanitizedClassName.push_back(c);
        }
    }
    if (sanitizedClassName.empty() || isdigit(static_cast<unsigned char>(sanitizedClassName[0]))) {
        sanitizedClassName = "Script_" + sanitizedClassName;
    }

    std::ofstream outFile(filePath);
    if (!outFile.is_open()) {
        spdlog::error("Failed to open file for writing script: {}", filePath.string());
        return false;
    }

    outFile << "#pragma once\n\n"
            << "#include \"systems/scriptSystem/Script.hpp\"\n"
            << "#include \"systems/scriptSystem/ScriptRegistry.hpp\"\n"
            << "#include \"systems/transformSystem/componets/TransformComponent.hpp\"\n\n"
            << "namespace engine::ecs {\n\n"
            << "    class " << sanitizedClassName << " : public Script {\n"
            << "    public:\n"
            << "        [[=Range{0.0f, 100.0f, 0.1f}, =Tooltip{\"Speed parameter\"}]]\n"
            << "        float speed = 1.0f;\n\n"
            << "        void OnStart() override {\n"
            << "            // Called once when the script starts\n"
            << "        }\n\n"
            << "        void OnUpdate(float deltaTime) override {\n"
            << "            // Called every frame\n"
            << "        }\n"
            << "    };\n\n"
            << "    REGISTER_SCRIPT(" << sanitizedClassName << ")\n\n"
            << "} // namespace engine::ecs\n";
    outFile.close();

    if (engine && engine->assetManagerInterface) {
        engine->assetManagerInterface->createAsset(am::AssetType::Script, filePath.string());
    }

    if (engine && engine->platform) {
        engine->platform->OpenFileInIDE(filePath.string());
    }

    spdlog::info("Created script asset: {}", filePath.string());
    return true;
}

std::filesystem::path EditorSystem::FindCMakeBuildDir() const
{
    std::filesystem::path exeDir;
#if defined(_WIN32)
    char modulePath[MAX_PATH];
    if (GetModuleFileNameA(nullptr, modulePath, MAX_PATH) > 0) {
        exeDir = std::filesystem::path(modulePath).parent_path();
    }
#endif
    if (exeDir.empty()) {
        exeDir = std::filesystem::current_path();
    }

    const std::vector<std::filesystem::path> candidates = {
        exeDir,
        exeDir.parent_path(),
        std::filesystem::current_path(),
        std::filesystem::current_path() / "out" / "build" / "debug",
        std::filesystem::current_path() / "out" / "build" / "release",
        std::filesystem::current_path() / "build"
    };

    for (const auto& candidate : candidates) {
        std::error_code ec;
        if (std::filesystem::exists(candidate / "CMakeCache.txt", ec)) {
            return std::filesystem::absolute(candidate);
        }
    }

    return exeDir;
}

void EditorSystem::TriggerScriptRebuild()
{
    if (isScriptCompiling.load()) {
        spdlog::warn("EditorSystem: Script compilation is already in progress");
        return;
    }

    isScriptCompiling = true;
    scriptCompilePendingReload = false;

    std::thread([this]() {
        std::filesystem::path buildDir = FindCMakeBuildDir();
        std::string cmd;
        if (!buildDir.empty()) {
            cmd = "cmake --build \"" + buildDir.string() + "\" --target gameplay_scripts 2>&1";
        } else {
            cmd = "cmake --build . --target gameplay_scripts 2>&1";
        }

        spdlog::info("EditorSystem: Spawning background script build: {}", cmd);

        std::string output;
        int exitCode = -1;

#if defined(_WIN32)
        FILE* pipe = _popen(cmd.c_str(), "r");
#else
        FILE* pipe = popen(cmd.c_str(), "r");
#endif
        if (pipe) {
            char buffer[512];
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                output += buffer;
            }
#if defined(_WIN32)
            exitCode = _pclose(pipe);
#else
            exitCode = pclose(pipe);
#endif
        } else {
            output = "Failed to spawn cmake build process.";
        }

        lastScriptBuildOutput = output;
        lastScriptBuildSuccess = (exitCode == 0);

        if (exitCode == 0) {
            spdlog::info("EditorSystem: Script compilation succeeded:\n{}", output);
            scriptCompilePendingReload = true;
        } else {
            spdlog::error("EditorSystem: Script compilation failed with exit code {}:\n{}", exitCode, output);
            showScriptBuildLogWindow = true;
        }

        isScriptCompiling = false;
    }).detach();
}

void EditorSystem::ImGuiScriptBuildLogWindow()
{
    if (!showScriptBuildLogWindow) return;

    ImGui::SetNextWindowSize(ImVec2(600, 350), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Script Build Output", &showScriptBuildLogWindow)) {
        if (isScriptCompiling.load()) {
            ImGui::BeginDisabled(true);
            ImGui::Button("Compiling...");
            ImGui::EndDisabled();
        } else {
            if (ImGui::Button("Recompile (Ctrl+R)")) {
                TriggerScriptRebuild();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            lastScriptBuildOutput.clear();
        }
        ImGui::SameLine();
        if (lastScriptBuildSuccess) {
            ImGui::TextColored(ImVec4(0.2f, 0.85f, 0.2f, 1.0f), "Status: Success");
        } else {
            ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "Status: Build Failed");
        }

        ImGui::Separator();
        ImGui::BeginChild("BuildOutputScrollRegion", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::TextUnformatted(lastScriptBuildOutput.c_str());
        ImGui::EndChild();
    }
    ImGui::End();
}

void EditorSystem::PollPluginBridge()
{
    // Poll command / control signals from IDE plugin
    std::vector<std::filesystem::path> cmdSignalPaths = {
        ".rv_control.signal",
        ".rv_control",
        "rv_control.signal",
        "rv_control",
        ".rv_command.signal",
        ".rv_command",
        "rv_command.signal",
        "rv_command",
        ".rv_state.signal",
        ".rv_state",
        "rv_state.signal",
        "rv_state",
        std::filesystem::temp_directory_path() / ".rv_control.signal",
        std::filesystem::temp_directory_path() / ".rv_control",
        std::filesystem::temp_directory_path() / "rv_control.signal",
        std::filesystem::temp_directory_path() / "rv_control",
        std::filesystem::temp_directory_path() / ".rv_command.signal",
        std::filesystem::temp_directory_path() / ".rv_command",
        std::filesystem::temp_directory_path() / "rv_command.signal",
        std::filesystem::temp_directory_path() / "rv_command",
        std::filesystem::temp_directory_path() / ".rv_state.signal",
        std::filesystem::temp_directory_path() / ".rv_state",
        std::filesystem::temp_directory_path() / "rv_state.signal",
        std::filesystem::temp_directory_path() / "rv_state"
    };

    for (const auto& path : cmdSignalPaths) {
        std::error_code ec;
        if (std::filesystem::exists(path, ec)) {
            uint64_t signalTs = 0;
            std::string content;
            std::ifstream file(path);
            if (file.is_open()) {
                std::stringstream buffer;
                buffer << file.rdbuf();
                content = buffer.str();
                file.close();
            }

            std::string cmd = content;
            try {
                auto colonPos = content.find(';');
                if (colonPos != std::string::npos) {
                    cmd = content.substr(0, colonPos);
                    signalTs = std::stoull(content.substr(colonPos + 1));
                } else if (content.find("\"timestamp\"") != std::string::npos) {
                    auto tsPos = content.find("\"timestamp\"");
                    auto numStart = content.find_first_of("0123456789", tsPos);
                    if (numStart != std::string::npos) {
                        auto numEnd = content.find_first_not_of("0123456789", numStart);
                        std::string tsStr = content.substr(numStart, numEnd - numStart);
                        signalTs = std::stoull(tsStr);
                    }
                }
            } catch (...) {}

            std::filesystem::remove(path, ec);

            if (signalTs == 0 || signalTs > lastProcessedStateTimestamp) {
                if (signalTs > 0) {
                    lastProcessedStateTimestamp = signalTs;
                }
                spdlog::info("EditorSystem: Received command '{}' from IDE plugin", cmd);

                std::string cmdUpper = cmd;
                for (char& c : cmdUpper) c = (char)std::toupper((unsigned char)c);

                if (cmdUpper.find("STOP") != std::string::npos) {
                    StopPlayMode();
                } else if (cmdUpper.find("TOGGLE_PLAY") != std::string::npos) {
                    TogglePlay();
                } else if (cmdUpper.find("TOGGLE_PAUSE") != std::string::npos) {
                    TogglePause();
                } else if (cmdUpper.find("PAUSE") != std::string::npos) {
                    SetPaused(true);
                } else if (cmdUpper.find("PLAY") != std::string::npos || cmdUpper.find("START") != std::string::npos || cmdUpper.find("RESUME") != std::string::npos) {
                    StartPlayMode();
                }
            }
        }
    }
}

} // namespace engine::ecs
