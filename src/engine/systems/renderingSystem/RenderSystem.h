//
// Created by redkc on 05/08/2025.
//

#ifndef RENDERSYSTEM_H
#define RENDERSYSTEM_H
#include "componets/Camera.hpp"
#include "componets/Light.hpp"
#include "componets/Model.hpp"
#include "ecs/System.h"


namespace engine::ecs
{
    class Scene;

    class RenderSystem :  public System<RenderSystem,Model,Camera,Light>
    {
    public:
        explicit RenderSystem(Scene* scene) : System(scene) {}
        void Update(float deltaTime) override;

    protected:
        void OnComponentAdded(ComponentID componentID, std::type_index type) override;
        void OnEntityRemoved(ComponentID componentID, std::type_index type) override {}
    };
}

#endif //RENDERSYSTEM_H
