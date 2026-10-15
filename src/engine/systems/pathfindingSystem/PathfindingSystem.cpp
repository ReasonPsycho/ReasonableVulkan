//
// Created by redkc on 14/10/2026.
//

#include "PathfindingSystem.hpp"
#include "ecs/Scene.h"

void engine::ecs::PathfindingSystem::Update(float deltaTime) {
    if (!scene) return;

    for (auto [entity, agent] : scene->GetActiveView<PathfindingAgent>()) {
        // Pathfinding agent update
    }
}

void engine::ecs::PathfindingSystem::OnComponentAdded(ComponentID componentID, std::type_index type) {

}
