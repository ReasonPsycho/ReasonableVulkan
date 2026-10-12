#pragma once

#include <typeindex>
#include "ecs/System.h"
#include "components/ScriptComponent.hpp"

namespace engine::ecs {

    class ScriptSystem : public System<ScriptSystem, ScriptComponent> {
    public:
        explicit ScriptSystem(Scene* scene);
        void Update(float deltaTime) override;

    protected:
        void OnComponentAdded(ComponentID componentID, std::type_index type) override {}
        void OnEntityRemoved(ComponentID componentID, std::type_index type) override;
    };

} // namespace engine::ecs
