#ifndef TRANSFORMSYSTEM_H
#define TRANSFORMSYSTEM_H

#include <typeindex>

#include "componets/Transform.hpp"
#include "../../ecs/System.h"

namespace engine::ecs
{
    struct TransformNode;
    class Scene;

    class TransformSystem :  public System<TransformSystem, Transform>
    {
    public:
        explicit TransformSystem(Scene* scene) : System(scene) {}
        void Update(float deltaTime) override;
        void ResetDirtyFlags();

    protected:
        void OnComponentAdded(ComponentID componentID, std::type_index type) override {}
        void OnEntityRemoved(ComponentID componentID, std::type_index type) override {}

    private:
        void UpdateTransformRecursive(Entity entity,const glm::mat4* parentMatrix, std::array<Transform, MAX_ENTITIES>& transforms);
    };
}

#endif //TRANSFORMSYSTEM_H
