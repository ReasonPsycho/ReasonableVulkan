//
// Created by redkc on 09/10/2025.
//

#include "EditorSystem.hpp"
#include <imgui.h>
#include "ecs/Scene.h"

void EditorSystem::ImGuiInspector()
{
    ImGui::Begin("Inspector");
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

    ImGui::End();
}

void engine::ecs::EditorSystem::Update(float deltaTime)
{
    ImGuiSceneGraph();
if (selectedEntity != std::numeric_limits<std::uint32_t>::max())
{
    ImGuiInspector();
}

    ImGui::ShowDemoWindow();
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