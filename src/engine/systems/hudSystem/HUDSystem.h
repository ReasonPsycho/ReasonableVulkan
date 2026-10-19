#pragma once

#include "ecs/System.h"
#include "components/RectTransformComponent.hpp"
#include "components/UIImageComponent.hpp"
#include <utility>

namespace engine::ecs {
    class Scene;

    class HUDSystem : public System<HUDSystem, RectTransformComponent, UIImageComponent> {
    public:
        explicit HUDSystem(Scene* scene) : System(scene) {}

        void Update(float deltaTime) override;

        std::pair<glm::vec2, glm::vec2> ComputeScreenRect(Entity entity, float screenWidth, float screenHeight) const;
    };
}
