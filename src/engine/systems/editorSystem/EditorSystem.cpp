//
// Created by redkc on 09/10/2025.
//

#include "EditorSystem.hpp"
#include <imgui.h>
#include <ImGuizmo.h>

#include "ecs/Scene.h"
#include <imgui_internal.h>
#include <SDL3/SDL_mouse.h>

#include "systems/renderingSystem/componets/Camera.hpp"
#include "PlatformInterface.hpp"
#include "systems/collisionSystem/CollisionSystem.hpp"

void EditorSystem::ImGuiInspector()
{
    ImGui::Begin("Inspector");
    if (selectedEntity != std::numeric_limits<std::uint32_t>::max())
    {
        auto name = GetEntityName(selectedEntity);
        ImGui::Text("Selected: %s", name.c_str());


        auto componentArrays = scene->GetComponentArrays();

        for (auto& [typeIndex, array] : componentArrays)
        {
            if (array.get()->HasComponentUntyped(selectedEntity))
            {
                registeredComponentTypes[typeIndex].showImGuiComponent(scene,&array.get()->GetComponentUntyped(selectedEntity));
            }
        }

        if (ImGui::Button("Add Component"))
        {
            ImGui::OpenPopup("Components List");
        }

        if (ImGui::BeginPopup("Components List"))
        {
            for (const auto& [typeIndex, info] : registeredComponentTypes)
            {
                if (info.displayName != boost::core::demangle(typeid(Transform).name()))
                {
                    if (ImGui::MenuItem(info.displayName.c_str()))
                    {
                        scene->AddComponent(selectedEntity,typeIndex);
                    }
                }
            }
            ImGui::EndPopup();
        }
    }

    ImGui::End();
}



void EditorSystem::ImGuiGizmo()
{
    if (selectedEntity != std::numeric_limits<std::uint32_t>::max())
    {
        auto& transform = scene->GetIntegralComponentArray<Transform>().get()->GetComponentFromEntity(selectedEntity);

        static ImGuizmo::OPERATION currentGizmoOperation(ImGuizmo::ROTATE);
        static ImGuizmo::MODE currentGizmoMode(ImGuizmo::WORLD);

        // Keyboard shortcuts for operation changes
        if (ImGui::IsKeyPressed(ImGuiKey_T))
            currentGizmoOperation = ImGuizmo::TRANSLATE;
        if (ImGui::IsKeyPressed(ImGuiKey_E))
            currentGizmoOperation = ImGuizmo::ROTATE;
        if (ImGui::IsKeyPressed(ImGuiKey_R))
            currentGizmoOperation = ImGuizmo::SCALE;

        // Operation radio buttons
        if (ImGui::RadioButton("Translate", currentGizmoOperation == ImGuizmo::TRANSLATE))
            currentGizmoOperation = ImGuizmo::TRANSLATE;
        ImGui::SameLine();
        if (ImGui::RadioButton("Rotate", currentGizmoOperation == ImGuizmo::ROTATE))
            currentGizmoOperation = ImGuizmo::ROTATE;
        ImGui::SameLine();
        if (ImGui::RadioButton("Scale", currentGizmoOperation == ImGuizmo::SCALE))
            currentGizmoOperation = ImGuizmo::SCALE;

        // Mode selection (except for Scale)
        if (currentGizmoOperation != ImGuizmo::SCALE)
        {
            if (ImGui::RadioButton("Local", currentGizmoMode == ImGuizmo::LOCAL))
                currentGizmoMode = ImGuizmo::LOCAL;
            ImGui::SameLine();
            if (ImGui::RadioButton("World", currentGizmoMode == ImGuizmo::WORLD))
                currentGizmoMode = ImGuizmo::WORLD;
        }

        // Snapping
        static bool useSnap = false;
        if (ImGui::IsKeyPressed(ImGuiKey_S))
            useSnap = !useSnap;
        ImGui::Checkbox("Use Snap", &useSnap);

        glm::vec3 snap(1.0f);
        if (currentGizmoOperation == ImGuizmo::TRANSLATE)
            snap = glm::vec3(0.5f); // Snap every 0.5 units for translation
        else if (currentGizmoOperation == ImGuizmo::ROTATE)
            snap = glm::vec3(45.0f); // Snap every 45 degrees for rotation
        else if (currentGizmoOperation == ImGuizmo::SCALE)
            snap = glm::vec3(0.1f); // Snap every 0.1 units for scale

        int windowPos[2];
        int windowSize[2];

        scene->engine.platform->GetWindowPosition(windowPos[0],windowPos[1]);
        scene->engine.platform->GetWindowSize(windowSize[0],windowSize[1]);

        // Get the viewport bounds for ImGuizmo
        ImGuiIO& io = ImGui::GetIO();
        ImGuizmo::SetRect(windowPos[0], windowPos[1], windowSize[0], windowSize[1]);

        // Convert glm matrices to float arrays for ImGuizmo
        float viewMatrix[16], projMatrix[16], modelMatrix[16];



        memcpy(viewMatrix, &camera.view[0][0], sizeof(float) * 16);
        memcpy(projMatrix, &camera.projection[0][0], sizeof(float) * 16);
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
                auto& parentTransform = scene->GetIntegralComponentArray<Transform>().get()->GetComponentFromEntity(it->second.parent);
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
    ImGui::Text("Toolbar Content");
    // Add your toolbar buttons/content here
    ImGui::End();
}

void EditorSystem::ImguiMenu()
{
    ImGui::Begin("Menu");
    ImGui::Text("Menu Content");
    // Add your menu content here
    ImGui::End();
}

EditorSystem::EditorSystem(Scene* scene): System(scene)
{
    Initialize();
}

void engine::ecs::EditorSystem::Update(float deltaTime)
{
    if (scene->engine.minimized)
        return;

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
        ImGui::DockBuilderDockWindow("Toolbar", dock_top);
        ImGui::DockBuilderDockWindow("Menu", dock_bottom);

        ImGui::DockBuilderFinish(dockspace_id);
    }

    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    ImguiToolbar();
    ImguiMenu();
    ImGuiSceneGraph();
    ImGuiInspector();
    ImGuiGizmo();

    ImGui::End();
}

void EditorSystem::SetEntityName(Entity entity, const std::string& name)
{
    if (name.empty()) {
        named_entities.erase(entity);
    } else {
        named_entities[entity] = name;
    }
}

std::string EditorSystem::GetEntityName(Entity entity) const
{
    auto it = named_entities.find(entity);
    return it != named_entities.end() ? "(#" + std::to_string(entity) + ") " + it->second : "(#" + std::to_string(entity) +") Entity";
}

void EditorSystem::Initialize()
{
        SetUpCameraControls(scene->engine.platform);

}


void EditorSystem::SetUpCameraControls(plt::PlatformInterface* platform)
{
    platform->SubscribeToEvent(plt::EventType::MouseButtonPressed, [this](const void* eventData) {
     const auto& mouseEvent = *static_cast<const plt::MouseButtonEvent*>(eventData);
     if (mouseEvent.button == SDL_BUTTON_RIGHT) {
         isRightMousePressed = true;
     } else if (mouseEvent.button == SDL_BUTTON_MIDDLE) {
         isMiddleMousePressed = true;
     }else
        if (mouseEvent.button == SDL_BUTTON_LEFT) {
            isLeftMousePressed = true;

            if (!ImGui::GetIO().WantCaptureMouse) {
                auto* collisionSystem = scene->GetSystem<CollisionSystem>().get();
                if (collisionSystem) {
                    int windowWidth, windowHeight;
                    scene->engine.platform->GetWindowSize(windowWidth, windowHeight);

                    Ray ray = collisionSystem->ScreenToWorldRay(camera, mouseEvent.x,
                                                                mouseEvent.y, windowWidth, windowHeight);

                    auto hit = collisionSystem->RayCastClosest(ray);
                    if (hit.has_value()) {
                        SetSelectedEntity(hit->entity);
                    } else {
                        SetSelectedEntity(std::numeric_limits<std::uint32_t>::max());
                    }
                }
            }
        }
    });

    platform->SubscribeToEvent(plt::EventType::MouseButtonReleased, [this](const void* eventData) {
        const auto& mouseEvent = *static_cast<const plt::MouseButtonEvent*>(eventData);
        if (mouseEvent.button == SDL_BUTTON_RIGHT) {
            isRightMousePressed = false;
        } else if (mouseEvent.button == SDL_BUTTON_MIDDLE) {
            isMiddleMousePressed = false;
        }else if (mouseEvent.button == SDL_BUTTON_LEFT) {
            isLeftMousePressed = false;
        }
    });

    // Set up mouse motion handler
    platform->SubscribeToEvent(plt::EventType::MouseMoved, [this, platform](const void* eventData) {
        const auto& motionEvent = *static_cast<const plt::MouseMoveEvent*>(eventData);

        if (isRightMousePressed) {
            // Orbit camera
            float sensitivity = 0.3f;
            cameraYaw += motionEvent.deltaX * sensitivity;
            cameraPitch += -motionEvent.deltaY * sensitivity;

            // Clamp pitch to prevent camera flipping
            cameraPitch = glm::clamp(cameraPitch, -89.0f, 89.0f);

            UpdateCameraPosition();
        }
        else if (isMiddleMousePressed) {
            // Pan camera
            float sensitivity = 0.001f * cameraDistance;
            glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0, 1, 0),
                cameraTransform.position - cameraTarget));
            glm::vec3 up = glm::cross(right, cameraTransform.position - cameraTarget);

            cameraTarget += right * (-motionEvent.deltaX * sensitivity);
            cameraTarget += up * (motionEvent.deltaY * sensitivity);

            UpdateCameraPosition();
        }
    });

    // Set up mouse wheel handler
    platform->SubscribeToEvent(plt::EventType::MouseScrolled, [this](const void* eventData) {
        const auto& wheelEvent = *static_cast<const plt::MouseScrollEvent*>(eventData);
        // Zoom camera
        float zoomSensitivity = 0.1f;
        cameraDistance = glm::max(0.1f, cameraDistance - wheelEvent.yOffset * zoomSensitivity);
        UpdateCameraPosition();
    });

    // Initialize camera position
    UpdateCameraPosition();
}

void EditorSystem::UpdateCameraPosition()
{
    // Calculate camera position based on spherical coordinates
    float x = cameraDistance * cos(glm::radians(cameraPitch)) * cos(glm::radians(cameraYaw));
    float y = cameraDistance * sin(glm::radians(cameraPitch));
    float z = cameraDistance * cos(glm::radians(cameraPitch)) * sin(glm::radians(cameraYaw));

    // Update camera transform
    cameraTransform.position = cameraTarget + glm::vec3(x, y, z);
    cameraTransform.rotation = glm::quatLookAt(
        glm::normalize(cameraTarget - cameraTransform.position),
        glm::vec3(0, 1, 0)
    );


    // Update camera matrices
    computeLocalMatrix(cameraTransform);
    cameraTransform.globalMatrix = cameraTransform.localMatrix;
    updateViewMatrix(camera, cameraTransform.globalMatrix);
}


void engine::ecs::EditorSystem::ImGuiSceneGraph()
{
    ImGui::Begin("Scene graph", nullptr, ImGuiWindowFlags_MenuBar);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::Button("New Entity"))
        {
            scene->CreateEntity();
        }

        if (ImGui::Button("Save Scene"))
        {
            scene->engine.SaveScene("scene.json");
        }

        if (ImGui::Button("Load Scene"))
        {
            scene->engine.LoadScene("scene.json");
        }

        ImGui::EndMenuBar();
    }

    for (Entity root : scene->rootEntities)
    {
        ImGuiGraphEntity(root);
    }

    // Handle dropping onto empty space (to make an entity a root)
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY"))
        {
            Entity droppedEntity = *(const Entity*)payload->Data;
            scene->RemoveParent(droppedEntity);
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::End();
}

void EditorSystem::ImGuiGraphEntity(Entity entity)
{
    std::string nameStr = GetEntityName(entity);
    auto it = scene->sceneGraph.find(entity);
    bool hasChildren = it != scene->sceneGraph.end() && !it->second.children.empty();

    ImGuiTreeNodeFlags flags = hasChildren ? 0 : ImGuiTreeNodeFlags_Leaf;
    flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

    // Add selection flag if this entity is selected
    if (entity == selectedEntity) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool nodeOpen = ImGui::TreeNodeEx(nameStr.c_str(), flags, "%s", nameStr.c_str());

    // Start drag operation
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
    {
        // Set payload to carry the entity index
        ImGui::SetDragDropPayload("SCENE_ENTITY", &entity, sizeof(Entity));
        ImGui::Text("Moving %s", nameStr.c_str());
        ImGui::EndDragDropSource();
    }

    // Handle incoming drag
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY"))
        {
            Entity droppedEntity = *(const Entity*)payload->Data;
            // Prevent dropping on itself or its children
            if (droppedEntity != entity)
            {
                scene->SetParent(droppedEntity, entity);
            }
        }
        ImGui::EndDragDropTarget();
    }

    // Handle selection when clicked
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        if (selectedEntity == entity)
        {
            selectedEntity = std::numeric_limits<std::uint32_t>::max();
        }
        else
        {
            selectedEntity = entity;
        }
    }

    if (nodeOpen) {
        ImGui::Indent();

        if (hasChildren)
        {
            for (Entity child : it->second.children)
            {
                ImGuiGraphEntity(child);
            }
        }

        ImGui::Unindent();
        ImGui::TreePop();
    }
}