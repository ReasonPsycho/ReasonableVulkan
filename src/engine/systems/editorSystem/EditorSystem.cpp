//
// Created by redkc on 09/10/2025.
//

#include "EditorSystem.hpp"
#include <imgui.h>
#include "ecs/Scene.h"

void engine::ecs::EditorSystem::Update(float deltaTime)
{
    ShowSceneGraph();
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

std::string_view EditorSystem::GetEntityName(Entity entity) const
{
    auto it = named_entities.find(entity);
    return it != named_entities.end() ? it->second : "Entity " + std::to_string(entity);
}

void engine::ecs::EditorSystem::ShowSceneGraph()
{
    ImGui::Begin("Scene graph");

    for (Entity root : scene->rootEntities)
    {
        ShowEntity(root);
    }

    ImGui::End();
}

void EditorSystem::ShowEntity(Entity entity)
{
    auto name = GetEntityName(entity);
    ImGui::TextUnformatted(name.data(), name.data() + name.length());

    auto it = scene->sceneGraph.find(entity);
    if (it != scene->sceneGraph.end())
    {
        for (Entity child : it->second.children)
        {
            ShowEntity(child);
        }
    }

}


