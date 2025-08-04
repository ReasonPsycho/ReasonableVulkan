#ifndef TRANSFORMSYSTEM_H
#define TRANSFORMSYSTEM_H

#include <unordered_map>

#include "Transform.h"
#include "ecs/System.h"

namespace engine::ecs
{
    struct TransformNode;
    class Scene;

    class TransformSystem :  public System<TransformSystem, Transform>
    {
    public:
        explicit TransformSystem(Scene* scene) : System(scene) {}
        void Update(float deltaTime) override;

    protected:
        void OnEntityAdded(Entity entity) override {}
        void OnEntityRemoved(Entity entity) override {}

    private:
        void UpdateTransformRecursive(Entity entity, std::array<Transform,MAX_ENTITIES>& transforms);
    };
}

#endif //TRANSFORMSYSTEM_H
