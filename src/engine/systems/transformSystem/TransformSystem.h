#ifndef TRANSFORMSYSTEM_H
#define TRANSFORMSYSTEM_H

#include <typeindex>
#include <vector>

#include "componets/TransformComponent.hpp"
#include "../../ecs/System.h"

namespace engine::ecs
{
    class Scene;

    class TransformSystem :  public System<TransformSystem, TransformComponent>
    {
    public:
        explicit TransformSystem(Scene* scene) : System(scene) {}
        void Update(float deltaTime) override;

    protected:
        void OnComponentAdded(ComponentID componentID, std::type_index type) override {}
        void OnEntityRemoved(ComponentID componentID, std::type_index type) override {}
    };
}

#endif //TRANSFORMSYSTEM_H
