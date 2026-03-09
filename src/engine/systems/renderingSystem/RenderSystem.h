//
// Created by redkc on 05/08/2025.
//

#ifndef RENDERSYSTEM_H
#define RENDERSYSTEM_H
#include "componets/CameraComponent.hpp"
#include "componets/LightComponent.hpp"
#include "componets/RendererComponent.hpp"
#include "ecs/System.h"


namespace engine::ecs
{
    class Scene;

    class RenderSystem :  public System<RenderSystem,RendererComponent,CameraComponent,LightComponent>
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
