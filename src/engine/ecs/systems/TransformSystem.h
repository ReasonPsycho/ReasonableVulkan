#ifndef TRANSFORMSYSTEM_H
#define TRANSFORMSYSTEM_H

#include "Transform.h"
#include "ecs/System.h"

namespace engine::ecs
{
    class Scene;

    class TransformSystem :  public System<TransformSystem, Transform>
    {
    public:


        void Update(float deltaTime) override;
        void OnAddEntity(Entity entity) override {}
        void OnRemoveEntity(Entity entity) override {}

    private:

        // Update and propagate isDirty flag
        void UpdateTransformRecursive(Entity entity, bool parentDirty);
    };
}

#endif //TRANSFORMSYSTEM_H
