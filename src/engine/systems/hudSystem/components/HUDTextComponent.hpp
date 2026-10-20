#pragma once

#include <string>
#include <utility>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/nil_generator.hpp>
#include "ecs/Component.hpp"
#include "ecs/utility/Reflection.hpp"
#include "HUDElementComponent.hpp"

namespace engine::ecs {

    struct HUDTextComponent : public Component {
        [[=Tooltip{"Screen anchor position"}]]
        int anchor{static_cast<int>(UIAnchor::TopLeft)};

        [[=Tooltip{"Offset in pixels from anchor point"}]]
        glm::vec2 offset{0.0f, 0.0f};

        [[=Tooltip{"Text string content"}]]
        std::string text{""};

        [[=Tooltip{"Font size / scaling in pixels"}]]
        float fontSize{16.0f};

        [[=Tooltip{"Character spacing in pixels"}]]
        float characterSpacing{1.0f};

        [[=Tooltip{"Line spacing multiplier"}]]
        float lineSpacing{1.0f};

        [[=Tooltip{"Text color (RGBA)"}]]
        glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};

        [[=Tooltip{"Font asset UUID"}, =LookupName{am::AssetType::Font}]]
        boost::uuids::uuid fontUuid{boost::uuids::nil_uuid()};

        [[=Tooltip{"Font texture asset UUID"}, =LookupName{am::AssetType::Texture}]]
        boost::uuids::uuid fontTextureUuid{boost::uuids::nil_uuid()};

        [[=Tooltip{"Layer depth / Z-order for sorting"}]]
        int layerDepth{0};

        [[=Tooltip{"Visibility flag"}]]
        bool isVisible{true};

        HUDTextComponent() = default;
        HUDTextComponent(std::string textStr, glm::vec2 off = {0.0f, 0.0f}, UIAnchor anch = UIAnchor::TopLeft, float size = 16.0f, glm::vec4 col = {1.0f, 1.0f, 1.0f, 1.0f})
            : anchor(static_cast<int>(anch)), offset(off), text(std::move(textStr)), fontSize(size), color(col) {}
    };

} // namespace engine::ecs
