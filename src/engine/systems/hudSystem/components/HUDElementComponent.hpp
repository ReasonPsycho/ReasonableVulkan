#pragma once

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/nil_generator.hpp>
#include "ecs/Component.hpp"
#include "ecs/utility/Reflection.hpp"

namespace engine::ecs {

    enum class UIAnchor {
        TopLeft = 0,
        TopCenter,
        TopRight,
        CenterLeft,
        Center,
        CenterRight,
        BottomLeft,
        BottomCenter,
        BottomRight
    };

    struct HUDElementComponent : public Component {
        [[=Tooltip{"Screen anchor position"}]]
        int anchor{static_cast<int>(UIAnchor::TopLeft)};

        [[=Tooltip{"Offset in pixels from anchor point"}]]
        glm::vec2 offset{0.0f, 0.0f};

        [[=Tooltip{"Dimensions of the HUD element in pixels"}]]
        glm::vec2 size{100.0f, 100.0f};

        [[=Tooltip{"Tint / Modulation color"}]]
        glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};

        [[=Tooltip{"Texture UV coordinates (uMin, vMin, uMax, vMax)"}]]
        glm::vec4 uvBounds{0.0f, 0.0f, 1.0f, 1.0f};

        [[=Tooltip{"Texture asset UUID"}, =LookupName{am::AssetType::Texture}]]
        boost::uuids::uuid textureUuid{boost::uuids::nil_uuid()};

        [[=Tooltip{"Layer depth / Z-order for sorting"}]]
        int layerDepth{0};

        [[=Tooltip{"Visibility flag"}]]
        bool isVisible{true};

        [[=Tooltip{"Whether this element can receive mouse click events"}]]
        bool isClickable{false};

        HUDElementComponent() = default;
        HUDElementComponent(boost::uuids::uuid texUuid, glm::vec2 sz = {100.0f, 100.0f}, glm::vec2 off = {0.0f, 0.0f}, UIAnchor anch = UIAnchor::TopLeft)
            : anchor(static_cast<int>(anch)), offset(off), size(sz), textureUuid(texUuid) {}
    };

} // namespace engine::ecs
