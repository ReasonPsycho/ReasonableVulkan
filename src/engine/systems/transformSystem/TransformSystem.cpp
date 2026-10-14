#include "TransformSystem.h"
#include "componets/TransformComponent.hpp"
#include "ecs/Scene.h"

using namespace engine::ecs;

void TransformSystem::Update(float /*deltaTime*/)
{
    auto& transforms = scene->GetIntegralComponentArray<TransformComponent>()->GetComponents();
    auto& flatHierarchy = scene->GetFlatHierarchy();

    for (size_t i = 0; i < flatHierarchy.size(); ++i)
    {
        auto& entry = flatHierarchy[i];
        TransformComponent& current = transforms[entry.entity];

        bool parentDirty = (entry.parentIndex >= 0) && flatHierarchy[entry.parentIndex].isDirty;
        bool isDirty = current.isDirty || parentDirty;
        entry.isDirty = isDirty;

        if (isDirty)
        {
            if (entry.parentIndex >= 0)
            {
                computeGlobalMatrix(current, transforms[entry.parent].globalMatrix);
            }
            else
            {
                computeGlobalMatrixRoot(current);
            }
        }
    }
}