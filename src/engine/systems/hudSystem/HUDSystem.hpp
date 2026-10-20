#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "components/HUDElementComponent.hpp"
#include "components/HUDTextComponent.hpp"
#include "assetDatas/FontData.h"
#include "ecs/System.h"
#include "GraphicsEngine.hpp"

namespace engine::ecs {
    class Scene;

    class HUDSystem : public System<HUDSystem, HUDElementComponent, HUDTextComponent> {
    public:
        explicit HUDSystem(Scene* scene);
        ~HUDSystem() override = default;

        void Update(float deltaTime) override;

        glm::vec2 CalculateAnchoredPosition(UIAnchor anchor, const glm::vec2& offset, const glm::vec2& size, const glm::vec2& screenSize) const;
        glm::vec2 CalculateTextDimensions(const std::string& text, float fontSize, float characterSpacing = 1.0f, const am::FontData* fontData = nullptr) const;

    protected:
        void OnComponentAdded(ComponentID componentID, std::type_index type) override {}
        void OnEntityRemoved(ComponentID componentID, std::type_index type) override {}
    };
} // namespace engine::ecs
