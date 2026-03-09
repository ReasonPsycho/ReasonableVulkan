#include "TransformSystem.h"
#include "componets/TransformComponent.hpp"
#include "ecs/Scene.h"

using namespace engine::ecs;

void TransformSystem::Update(float /*deltaTime*/)
{
    auto& transforms = scene->GetIntegralComponentArray<TransformComponent>()->GetComponents();

    for (Entity root : scene->rootEntities)
    {
        UpdateTransformRecursive(root, nullptr, transforms);
    }
}

void TransformSystem::UpdateTransformRecursive(Entity entity,const glm::mat4* parentMatrix, std::array<TransformComponent, MAX_ENTITIES>& transforms)
{
    TransformComponent& current = transforms[entity];
    bool isDirty = current.isDirty || (parentMatrix != nullptr);

    if (isDirty)
    {
        computeGlobalMatrix(current, parentMatrix ? *parentMatrix : glm::mat4(1.0f));
    }

    auto it = scene->sceneGraph.find(entity);
    if (it != scene->sceneGraph.end())
    {
        for (Entity child : it->second.children)
        {
            UpdateTransformRecursive(child, &current.globalMatrix, transforms);
        }
    }
}