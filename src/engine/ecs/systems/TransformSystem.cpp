#include "TransformSystem.h"
#include "Transform.h"
#include "ecs/Types.h"
#include "ecs/Scene.h"


void TransformSystem::Update(float /*deltaTime*/)
{
    for (const auto& entity : scene->rootEntities)
    {
        auto transformNode = scene->sceneGraph[entity];
        for (Entity child : transformNode.children)
        {
            UpdateTransformRecursive(child,scene->GetIntegralComponentArray<Transform>()->GetComponents());
        }
    }
}

void TransformSystem::UpdateTransformRecursive(Entity entity, std::array<Transform, MAX_ENTITIES>& transforms)
{
    auto& t = transforms[entity];
    auto it = scene->sceneGraph.find(entity);

    const auto& parentTransform = transforms[it->second.parent];
    computeGlobalMatrix(t, parentTransform.globalMatrix);

    // If this entity was not dirty, skip children
    if (!t.isDirty)
        return;

    // Recurse into dirty children only
    if (it != scene->sceneGraph.end())
    {
        for (Entity child : it->second.children)
        {
            auto& childTransform = transforms[entity];
            if (childTransform.isDirty)
                UpdateTransformRecursive(child, transforms);
        }
    }
}