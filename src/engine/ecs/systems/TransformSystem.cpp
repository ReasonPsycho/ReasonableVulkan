#include "TransformSystem.h"

#include "Transform.h"
#include "ecs/ComponentArray.h"
#include "ecs/Scene.h"
#include "ecs/Types.h"

using namespace engine::ecs;

void TransformSystem::Update(float /*deltaTime*/)
{
    for (const auto& entity : scene->rootEntities)
    {
        UpdateTransformRecursive(entity, false);
    }
}

void TransformSystem::UpdateTransformRecursive(Entity entity, bool parentDirty)
{
    auto& transform = scene->GetComponent<Transform>(entity);

    // If parent is dirty or this transform is dirty, update
    bool isThisDirty = transform.isDirty || parentDirty;

    if (isThisDirty)
    {
        transform.matrix = getLocalModelMatrix(transform);
        transform.isDirty = false;
    }

    for (Entity child : scene->GetChildren(entity))
    {
        UpdateTransformRecursive(child, isThisDirty);
    }
}
