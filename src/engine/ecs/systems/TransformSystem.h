#ifndef TRANSFORMSYSTEM_H
#define TRANSFORMSYSTEM_H

#include "ecs/System.h"

namespace engine::ecs
{
    class Scene;
    struct Transform;

    class TransformSystem :  public System<TransformSystem, Transform>
    {
    public:
        explicit TransformSystem(Scene* scene) : scene(scene) {}

        void Update(float deltaTime) override;

    protected:
        void AddEntity(Entity entity) override {}
        void RemoveEntity(Entity entity) override {}

    private:
        Scene* scene;

        // Update and propagate isDirty flag
        void UpdateTransformRecursive(Entity entity, bool parentDirty);
    };
}

#endif //TRANSFORMSYSTEM_H
