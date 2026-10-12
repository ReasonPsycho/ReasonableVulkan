#pragma once

#include "systems/scriptSystem/Script.hpp"
#include "systems/scriptSystem/ScriptRegistry.hpp"
#include "systems/transformSystem/componets/TransformComponent.hpp"

namespace engine::ecs {

    class MoveLeftScript : public Script {
    public:
        [[=Range{0.0f, 50.0f, 0.1f}, =Tooltip{"Movement speed in units per second"}]]
        float speed = 2.0f;

        [[=Tooltip{"If true, move along the object's local left axis; otherwise along world -X"}]]
        bool useLocalSpace = true;

        void OnStart() override {
            // Optional initialization logic (e.g. logging or caching references)
        }

        void OnUpdate(float deltaTime) override {
            auto& transform = GetComponent<TransformComponent>();

            glm::vec3 leftDirection;
            if (useLocalSpace) {
                // Local left is the negative right vector of the local transform
                leftDirection = (transform.rotation * glm::vec3(0.0f, 1.0f, 0.0f));
            } else {
                // World left (-X axis)
                leftDirection = glm::vec3(1.0f, 0.0f, 0.0f);
            }

            // Apply movement scaled by deltaTime
            glm::vec3 newPosition = transform.position + leftDirection * speed * deltaTime;
            setLocalPosition(transform, newPosition);
        }
    };

    // Register script so it appears in the Editor Inspector dropdown and supports JSON serialization
    REGISTER_SCRIPT(MoveLeftScript)

} // namespace engine::ecs
