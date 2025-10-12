//
// Created by redkc on 09/10/2025.
//

#include "EditorSystem.hpp"
#include <imgui.h>
#include "ecs/Scene.h"
#include <imgui_internal.h>

void EditorSystem::ImGuiInspector()
{
    ImGui::Begin("Inspector");
    if (selectedEntity != std::numeric_limits<std::uint32_t>::max())
    {
        auto name = GetEntityName(selectedEntity);
        ImGui::Text("Selected: %s", name.c_str());


        auto componentArrays = scene->GetComponentArrays();

        for (auto& [_, array] : componentArrays)
        {
            if (array.get()->HasComponentUntyped(selectedEntity))
            {


                array.get()->GetComponentUntyped(selectedEntity).ImGuiComponent();
            }
        }
    }
    ImGui::End();
}





void engine::ecs::EditorSystem::Update(float deltaTime)
{
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

    // Create the windows
    ImGui::Begin("Toolbar");
    ImGui::Text("Toolbar Content");
    // Add your toolbar buttons/content here
    ImGui::End();

    ImGui::Begin("Menu");
    ImGui::Text("Menu Content");
    // Add your menu content here
    ImGui::End();

    ImGuiSceneGraph();
    ImGuiInspector();

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

void engine::ecs::EditorSystem::ImGuiSceneGraph()
{
    ImGui::Begin("Scene graph");

    for (Entity root : scene->rootEntities)
    {
        ImGuiGraphEntity(root);
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

    // Handle selection when clicked
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        if (selectedEntity == entity)
        {
            selectedEntity = std::numeric_limits<std::uint32_t>::max();
        }else
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