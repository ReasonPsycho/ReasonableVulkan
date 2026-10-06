//
// Created by redkc on 09/10/2025.
//

#include "EditorSystem.hpp"
#include <imgui.h>
#include <ImGuizmo.h>

#include "ecs/Scene.h"
#include "Engine.h"
#include <imgui_internal.h>
#include <SDL3/SDL_mouse.h>

#include "Asset.hpp"
#include "systems/renderingSystem/componets/CameraComponent.hpp"
#include "systems/renderingSystem/componets/MeshComponent.hpp"
#include "systems/renderingSystem/componets/RendererComponent.hpp"
#include "systems/renderingSystem/componets/LightComponent.hpp"
#include "systems/transformSystem/componets/TransformComponent.hpp"
#include "ecs/NameComponent.hpp"
#include "ecs/TagComponent.hpp"
#include "PlatformInterface.hpp"
#include "assetDatas/MeshData.h"
#include "assetDatas/ModelData.h"
#include "systems/collisionSystem/CollisionSystem.hpp"
#include "systems/gizmoSystem/GizmoSystem.hpp"
#include "RenderDocManager.hpp"

namespace engine::ecs {

Scene* EditorSystem::GetTargetScene() const
{
    if (auto scn = selectedScene.lock()) {
        return scn.get();
    }
    if (engine) {
        auto topEditable = engine->GetTopEditableScene();
        if (topEditable) return topEditable.get();

        auto activeScenes = engine->GetActiveScenes();
        if (!activeScenes.empty() && activeScenes[0]) {
            return activeScenes[0].get();
        }
    }
    return nullptr;
}

void EditorSystem::ImGuiInspector()
{
    ImGui::Begin("Inspector");
    Scene* scene = GetTargetScene();
    if (scene && selectedEntity != std::numeric_limits<std::uint32_t>::max())
    {
        if (!scene->IsEntityActive(selectedEntity) && !scene->HasComponent<TransformComponent>(selectedEntity))
        {
            selectedEntity = std::numeric_limits<std::uint32_t>::max();
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

        // 1. Entity Header: Active toggle, Name, Entity ID
        bool active = scene->IsEntityActive(selectedEntity);
        if (ImGui::Checkbox("##EntityActive", &active))
        {
            scene->SetEntityActive(selectedEntity, active);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip(active ? "Entity is Active" : "Entity is Inactive");
        ImGui::SameLine();

        char nameBuf[256];
        std::string rawName = GetEntityRawName(selectedEntity, scene);
        std::snprintf(nameBuf, sizeof(nameBuf), "%s", rawName.c_str());
        float idWidth = 70.0f;
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - idWidth);
        if (ImGui::InputText("##EntityName", nameBuf, sizeof(nameBuf)))
        {
            SetEntityName(selectedEntity, nameBuf, scene);
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
            auto& array = componentArrays[componentToRemove];
            array->RemoveComponentUntyped(selectedEntity);
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
                        scene->AddComponent(selectedEntity, typeIndex);
                    }
                }
            }

            if (!anyAvailable)
            {
                ImGui::TextDisabled("No more components available");
            }

            ImGui::EndPopup();
        }

        if (!isEditable)
        {
            ImGui::EndDisabled();
        }
    }
    else
    {
        ImGui::TextDisabled("No entity selected");
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
    if (selectedEntity != std::numeric_limits<std::uint32_t>::max() && scene->HasComponent<TransformComponent>(selectedEntity))
    {
        auto& transform = scene->GetIntegralComponentArray<TransformComponent>().get()->GetComponentFromEntity(selectedEntity);

        static ImGuizmo::OPERATION currentGizmoOperation(ImGuizmo::ROTATE);
        static ImGuizmo::MODE currentGizmoMode(ImGuizmo::WORLD);

        // Keyboard shortcuts for operation changes
        if (ImGui::IsKeyPressed(ImGuiKey_T))
            currentGizmoOperation = ImGuizmo::TRANSLATE;
        if (ImGui::IsKeyPressed(ImGuiKey_E))
            currentGizmoOperation = ImGuizmo::ROTATE;
        if (ImGui::IsKeyPressed(ImGuiKey_R))
            currentGizmoOperation = ImGuizmo::SCALE;

        // Snapping
        static bool useSnap = false;
        if (ImGui::IsKeyPressed(ImGuiKey_S))
            useSnap = !useSnap;

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
            }
            ImGui::End();
        }

        glm::vec3 snap(1.0f);
        if (currentGizmoOperation == ImGuizmo::TRANSLATE)
            snap = glm::vec3(0.5f); // Snap every 0.5 units for translation
        else if (currentGizmoOperation == ImGuizmo::ROTATE)
            snap = glm::vec3(45.0f); // Snap every 45 degrees for rotation
        else if (currentGizmoOperation == ImGuizmo::SCALE)
            snap = glm::vec3(0.1f); // Snap every 0.1 units for scale

        // Get the viewport bounds for ImGuizmo
        ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);

        // Convert glm matrices to float arrays for ImGuizmo
        float viewMatrix[16], projMatrix[16], modelMatrix[16];

        memcpy(viewMatrix, &camState.camera.view[0][0], sizeof(float) * 16);
        memcpy(projMatrix, &camState.camera.projection[0][0], sizeof(float) * 16);
        projMatrix[5] *= -1.0f; // Invert Y back for ImGuizmo (since Vulkan Y was inverted in projection)
        memcpy(modelMatrix, &transform.globalMatrix[0][0], sizeof(float) * 16);

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
            glm::mat4 newGlobalMatrix;
            memcpy(&newGlobalMatrix[0][0], modelMatrix, sizeof(float) * 16);

            // If we have a parent, we need to convert global to local
            auto it = scene->sceneGraph.find(selectedEntity);
            if (it != scene->sceneGraph.end() && it->second.parent != MAX_ENTITIES)
            {
                auto& parentTransform = scene->GetIntegralComponentArray<TransformComponent>().get()->GetComponentFromEntity(it->second.parent);
                setLocalMatrixFromGlobal(transform, newGlobalMatrix, parentTransform.globalMatrix);
            }
            else
            {
                // No parent, just set the local matrix directly
                setLocalMatrix(transform, newGlobalMatrix);
            }
        }
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

    ImGui::End();
}

EditorSystem::EditorSystem(::engine::Engine* engine) : engine(engine)
{
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

    if (ImGui::IsKeyPressed(ImGuiKey_F11)) {
        rd::RenderDocManager::getInstance().requestCapture();
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
        ImGui::DockBuilderDockWindow("Toolbar", dock_top);
        ImGui::DockBuilderDockWindow("Menu", dock_bottom);
        ImGui::DockBuilderDockWindow("Editor", dock_main_id);
        ImGui::DockBuilderDockWindow("Game", dock_main_id);

        if (engine) {
            auto currentActiveScenes = engine->GetActiveScenes();
            for (const auto& scn : currentActiveScenes) {
                if (scn) {
                    ImGui::DockBuilderDockWindow(("Editor (" + scn->GetName() + ")").c_str(), dock_main_id);
                }
            }
        }

        ImGui::DockBuilderFinish(dockspace_id);
    }

    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    // 1. Render all Editor windows (one per active scene)
    auto activeScenes = engine->GetActiveScenes();
    for (size_t sceneIdx = 0; sceneIdx < activeScenes.size(); ++sceneIdx) {
        auto currentScene = activeScenes[sceneIdx];
        if (!currentScene) continue;

        std::string windowTitle = (activeScenes.size() == 1) ? "Editor" : ("Editor (" + currentScene->GetName() + ")");
        ImGuiDockNode* centralNode = ImGui::DockBuilderGetCentralNode(dockspace_id);
        ImGuiID centerId = centralNode ? centralNode->ID : dockspace_id;
        ImGui::SetNextWindowDockID(centerId, ImGuiCond_FirstUseEver);
        ImGui::Begin(windowTitle.c_str());
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

            // Selection (Left Click)
            if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsOver()) {
                selectedScene = currentScene;
                auto* collisionSystem = currentScene->GetSystem<CollisionSystem>().get();
                if (collisionSystem) {
                    ImVec2 mousePos = ImGui::GetMousePos();
                    Ray ray = collisionSystem->ScreenToWorldRay(camState.camera, mousePos.x - viewportPos.x,
                                                                mousePos.y - viewportPos.y, viewportPanelSize.x, viewportPanelSize.y);

                    auto gizmoSystem = currentScene->GetSystem<GizmoSystem>();
                    if (gizmoSystem) {
                        gizmoSystem->DrawRay(ray.origin, ray.direction * 100.0f, glm::vec3(1.0f, 0.0f, 0.0f), 5);
                    }

                    auto hit = collisionSystem->RayCastClosest(ray);
                    if (hit.has_value()) {
                        SetSelectedEntity(hit->entity);
                        ImGui::SetWindowFocus("Inspector");
                    } else {
                        SetSelectedEntity(std::numeric_limits<std::uint32_t>::max());
                    }
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
                camState.cameraYaw += io.MouseDelta.x * sensitivity;
                camState.cameraPitch += -io.MouseDelta.y * sensitivity;
                camState.cameraPitch = glm::clamp(camState.cameraPitch, -89.0f, 89.0f);
                camState.UpdateCameraPosition();
            }

            // Pan camera
            if (camState.isMiddleMousePressed) {
                float sensitivity = 0.001f * camState.cameraDistance;
                glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0, 1, 0),
                    camState.cameraTransform.position - camState.cameraTarget));
                glm::vec3 up = glm::cross(right, camState.cameraTransform.position - camState.cameraTarget);

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
                        std::filesystem::path dp((const char*)curPayload->Data);
                        draggedAssetName = dp.filename().string();
                        draggedAssetExt = dp.extension().string();
                    }

                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AM_FILE_PATH"))
                    {
                        std::string filePath((const char*)payload->Data);
                        std::filesystem::path p(filePath);
                        auto ext = p.extension().string();
                        if (ext == ".scene") {
                            auto uuid = engine->assetManagerInterface->getAssetUuidByPath(p);
                            if (uuid) engine->LoadScene(uuid.value());
                        } else if (ext == ".fbx" || ext == ".obj" || ext == ".model" || ext == ".mesh") {
                            auto uuid = engine->assetManagerInterface->getAssetUuidByPath(p);
                            if (!uuid) uuid = engine->assetManagerInterface->registerAsset(p.string());
                            if (uuid) {
                                Entity entity = currentScene->CreateEntity(p.stem().string());
                                MeshComponent meshComp(uuid.value());
                                currentScene->AddComponent<MeshComponent>(entity, meshComp);
                                auto shaderOpt = engine->assetManagerInterface->getAssetUuid("pbrShader");
                                if (shaderOpt) {
                                    RendererComponent rendererComp(shaderOpt.value());
                                    currentScene->AddComponent<RendererComponent>(entity, rendererComp);
                                }
                            }
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
                    } else if (draggedAssetExt == ".fbx" || draggedAssetExt == ".obj" || draggedAssetExt == ".model" || draggedAssetExt == ".mesh") {
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
            if (gameTextureId) {
                ImGui::Image((ImTextureID)gameTextureId, gameViewportPanelSize);
            }
            ImGui::End();
        }
    }

    ImguiToolbar();

    if (engine->assetManagerInterface) {
        engine->assetManagerInterface->ImguiFileBrowser("Menu");
        engine->assetManagerInterface->ImguiFileInspector("File Inspector");
    }

    ImGuiSceneGraph();
    ImGuiInspector();
    ImGuiSystemSettings();

    ImGui::End();
}

void EditorSystem::SetEntityName(Entity entity, const std::string& name, Scene* targetScene)
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

std::string EditorSystem::GetEntityRawName(Entity entity, Scene* targetScene) const
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

std::string EditorSystem::GetEntityName(Entity entity, Scene* targetScene) const
{
    return "(#" + std::to_string(entity) + ") " + GetEntityRawName(entity, targetScene);
}

void EditorSystem::Initialize()
{
    ForEachType<EngineComponents>([this]<typename T>() {
        this->RegisterComponentType<T>();
    });

    SetUpCameraControls();

    if (engine && engine->assetManagerInterface && engine->graphicsEngine) {
        auto skyboxModelData = engine->assetManagerInterface->getAssetData<am::ModelData>("skyboxModel");
        if (skyboxModelData && !skyboxModelData->rootNode.mChildren.empty() && !skyboxModelData->rootNode.mChildren[0].meshes.empty()) {
            auto skyboxMeshData = engine->assetManagerInterface->getAssetData<am::MeshData>(skyboxModelData->rootNode.mChildren[0].meshes[0].get()->id);
            /*if (skyboxMeshData && skyboxMeshData->material) { //TODO add an seperate skybox for the editor
                camera.skyboxMaterialId = skyboxMeshData->material.get()->id;
                camera.runtimeSkyboxMaterialHandle = engine->graphicsEngine->loadMaterial(camera.skyboxMaterialId);
            }*/
        }

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
    ImGui::Begin("Scene graph", nullptr, ImGuiWindowFlags_MenuBar);

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
                    selectedEntity = std::numeric_limits<std::uint32_t>::max();
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
        if (!scn) continue;
        ImGui::PushID(sceneName.c_str());
        bool isCurrentActive = scn->IsActive();
        if (ImGui::Checkbox("##active", &isCurrentActive))
        {
            scn->SetActive(isCurrentActive);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle Scene Active");
        ImGui::SameLine();

        ImGuiTreeNodeFlags sceneFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen;
        if (scn.get() == targetScene)
        {
            sceneFlags |= ImGuiTreeNodeFlags_Selected;
        }

        std::string displayName = sceneName + (scn->IsEditable() ? "" : " (Read-Only)");
        bool sceneNodeOpen = ImGui::TreeNodeEx((void*)scn.get(), sceneFlags, "%s", displayName.c_str());
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        {
            selectedScene = scn;
        }

        // Context menu for the scene
        if (ImGui::BeginPopupContextItem())
        {
            if (scn->IsEditable())
            {
                if (ImGui::MenuItem("Create Entity"))
                {
                    selectedScene = scn;
                    scn->CreateEntity();
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
            auto rootsCopy = scn->rootEntities;
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
                selectedEntity = std::numeric_limits<std::uint32_t>::max();
                renamingEntity = std::numeric_limits<std::uint32_t>::max();
            }
        }
        engine->CloseScene(sceneToClose);
    }

    if (targetScene && targetScene->IsEditable() && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && selectedEntity != std::numeric_limits<std::uint32_t>::max() && renamingEntity == std::numeric_limits<std::uint32_t>::max())
    {
        if (ImGui::IsKeyPressed(ImGuiKey_F2))
        {
            renamingEntity = selectedEntity;
            std::snprintf(renameBuf, sizeof(renameBuf), "%s", GetEntityRawName(selectedEntity, targetScene).c_str());
            renameFocusRequested = true;
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
                Entity droppedEntity = *(const Entity*)payload->Data;
                targetScene->RemoveParent(droppedEntity);
            }
            else if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AM_FILE_PATH"))
            {
                std::string filePath((const char*)payload->Data);
                std::filesystem::path p(filePath);
                auto ext = p.extension().string();
                if (ext == ".scene") {
                    auto uuid = engine->assetManagerInterface->getAssetUuidByPath(p);
                    if (uuid) engine->LoadScene(uuid.value());
                } else if (ext == ".fbx" || ext == ".obj" || ext == ".model" || ext == ".mesh") {
                    auto uuid = engine->assetManagerInterface->getAssetUuidByPath(p);
                    if (!uuid) uuid = engine->assetManagerInterface->registerAsset(p.string());
                    if (uuid) {
                        Entity entity = targetScene->CreateEntity(p.stem().string());
                        MeshComponent meshComp(uuid.value());
                        targetScene->AddComponent<MeshComponent>(entity, meshComp);
                        auto shaderOpt = engine->assetManagerInterface->getAssetUuid("pbrShader");
                        if (shaderOpt) {
                            RendererComponent rendererComp(shaderOpt.value());
                            targetScene->AddComponent<RendererComponent>(entity, rendererComp);
                        }
                    }
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
    if (targetScene && targetScene->IsEditable() && ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        if (ImGui::MenuItem("Create Entity"))
        {
            targetScene->CreateEntity();
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

void EditorSystem::ImGuiGraphEntity(Scene* currentScene, Entity entity)
{
    if (!currentScene) return;
    std::string nameStr = GetEntityName(entity, currentScene);
    auto it = currentScene->sceneGraph.find(entity);
    bool hasChildren = it != currentScene->sceneGraph.end() && !it->second.children.empty();

    ImGuiTreeNodeFlags flags = hasChildren ? 0 : ImGuiTreeNodeFlags_Leaf;
    flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

    Scene* targetScene = GetTargetScene();
    if (entity == selectedEntity && currentScene == targetScene) {
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
            SetEntityName(entity, renameBuf, currentScene);
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
                SetEntityName(entity, renameBuf, currentScene);
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

    // Context menu on entity
    if (ImGui::BeginPopupContextItem())
    {
        if (currentScene->IsEditable())
        {
            if (ImGui::MenuItem("Rename", "F2"))
            {
                renamingEntity = entity;
                std::snprintf(renameBuf, sizeof(renameBuf), "%s", GetEntityRawName(entity, currentScene).c_str());
                renameFocusRequested = true;
            }
            if (ImGui::MenuItem("Create Child Entity"))
            {
                currentScene->CreateEntity("Child Entity", entity);
            }
            if (ImGui::MenuItem(isActive ? "Disable Entity" : "Enable Entity"))
            {
                currentScene->SetEntityActive(entity, !isActive);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete Entity"))
            {
                currentScene->DestroyEntity(entity);
                if (selectedEntity == entity)
                {
                    selectedEntity = std::numeric_limits<std::uint32_t>::max();
                }
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
        else
        {
            ImGui::TextDisabled("Scene is Read-Only");
        }
        ImGui::EndPopup();
    }

    // Start drag operation
    if (currentScene->IsEditable() && !isRenaming && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
    {
        // Set payload to carry the entity index
        ImGui::SetDragDropPayload("SCENE_ENTITY", &entity, sizeof(Entity));
        ImGui::BeginGroup();
        ImFont* iconFont = (ImGui::GetIO().Fonts->Fonts.Size > 1) ? ImGui::GetIO().Fonts->Fonts[1] : ImGui::GetFont();
        float dragIconSize = (ImGui::GetIO().Fonts->Fonts.Size > 1 ? 24.0f : ImGui::GetFontSize() * 1.5f);
        ImGui::PushFont(iconFont);
        ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.0f, 1.0f), "%s", ICON_FA_CUBE);
        ImGui::PopFont();
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::TextUnformatted(nameStr.c_str());
        ImGui::TextDisabled("Entity #%u - Drag to reparent", entity);
        ImGui::EndGroup();
        ImGui::EndGroup();
        ImGui::EndDragDropSource();
    }

    // Handle incoming drag
    bool isNodeDropTarget = false;
    if (currentScene->IsEditable() && ImGui::BeginDragDropTarget())
    {
        isNodeDropTarget = true;
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY"))
        {
            Entity droppedEntity = *(const Entity*)payload->Data;
            // Prevent dropping on itself or its children
            if (droppedEntity != entity)
            {
                currentScene->SetParent(droppedEntity, entity);
            }
        }
        else if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AM_FILE_PATH"))
        {
            std::string filePath((const char*)payload->Data);
            std::filesystem::path p(filePath);
            auto ext = p.extension().string();
            if (ext == ".scene") {
                auto uuid = engine->assetManagerInterface->getAssetUuidByPath(p);
                if (uuid) engine->LoadScene(uuid.value());
            } else if (ext == ".fbx" || ext == ".obj" || ext == ".model" || ext == ".mesh") {
                auto uuid = engine->assetManagerInterface->getAssetUuidByPath(p);
                if (!uuid) uuid = engine->assetManagerInterface->registerAsset(p.string());
                if (uuid) {
                    Entity childEntity = currentScene->CreateEntity(p.stem().string(), entity);
                    MeshComponent meshComp(uuid.value());
                    currentScene->AddComponent<MeshComponent>(childEntity, meshComp);
                    auto shaderOpt = engine->assetManagerInterface->getAssetUuid("pbrShader");
                    if (shaderOpt) {
                        RendererComponent rendererComp(shaderOpt.value());
                        currentScene->AddComponent<RendererComponent>(childEntity, rendererComp);
                    }
                }
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
    if (!isRenaming && ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        selectedScene = currentScene->engine.GetScene(currentScene->GetName());
        if (selectedEntity == entity)
        {
            selectedEntity = std::numeric_limits<std::uint32_t>::max();
        }
        else
        {
            selectedEntity = entity;
            ImGui::SetWindowFocus("Inspector");
        }
    }

    if (nodeOpen) {
        ImGui::Indent();
        auto itNode = currentScene->sceneGraph.find(entity);
        if (itNode != currentScene->sceneGraph.end()) {
            auto childrenCopy = itNode->second.children;
            for (Entity child : childrenCopy) {
                ImGuiGraphEntity(currentScene, child);
            }
        }
        ImGui::Unindent();
        ImGui::TreePop();
    }
}

} // namespace engine::ecs
