//
// Created by redkc on 24/10/2025.
//

#ifndef REASONABLEVULKAN_LIGHTSYSTEM_HPP
#define REASONABLEVULKAN_LIGHTSYSTEM_HPP
#include <typeindex>

#include "components/SpotLight.hpp"
#include "components/DirectionalLight.hpp"
#include "components/PointLight.hpp"

#include "ecs/System.h"

namespace engine::ecs
{
    class Scene;

    class LightSystem : public System<LightSystem, SpotLight, DirectionalLight, PointLight>
    {

    public:
        explicit LightSystem(Scene* scene) : System(scene){};

        void Update(float deltaTime) override;

    protected:
        void OnComponentAdded(ComponentID componentID, std::type_index type) override{};
        void OnEntityRemoved(ComponentID componentID, std::type_index type) override {};

    };
}



#endif //REASONABLEVULKAN_LIGHTSYSTEM_HPP