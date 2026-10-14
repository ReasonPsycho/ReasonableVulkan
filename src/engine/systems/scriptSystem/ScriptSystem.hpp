#pragma once

#include <typeindex>
#include <rapidjson/document.h>
#include "ecs/System.h"
#include "components/ScriptComponent.hpp"

namespace engine::ecs {

    class ScriptSystem : public System<ScriptSystem, ScriptComponent> {
    public:
        explicit ScriptSystem(Scene* scene);
        void Update(float deltaTime) override;

        rapidjson::Document BeginHotReload();
        void EndHotReload(const rapidjson::Document& snapshot);
        bool PerformHotReload();

        static bool PerformHotReloadForScene(Scene* scene);

    protected:
        void OnComponentAdded(ComponentID componentID, std::type_index type) override {}
        void OnEntityRemoved(ComponentID componentID, std::type_index type) override;
    };

} // namespace engine::ecs
