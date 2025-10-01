//
// Created by redkc on 05/08/2025.
//

#ifndef RENDERSYSTEM_H
#define RENDERSYSTEM_H
#include "componets/Camera.h"
#include "componets/Model.hpp"
#include "ecs/System.h"

#endif //RENDERSYSTEM_H

namespace engine::ecs
{
    class Scene;

    class RenderSystem :  public System<RenderSystem,Camera, Model>
    {
    public:
        explicit RenderSystem(Scene* scene) : System(scene) {}
        void Update(float deltaTime) override;

    protected:
        void OnEntityAdded(Entity entity) override {}
        void OnEntityRemoved(Entity entity) override {}

    };
}