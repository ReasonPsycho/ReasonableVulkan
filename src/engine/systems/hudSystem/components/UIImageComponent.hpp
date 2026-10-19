#pragma once
#include <glm/vec4.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include "Handle.hpp"
#include "ecs/Component.hpp"
#include "ecs/utility/Reflection.hpp"

namespace engine::ecs {
    struct UIImageComponent : public Component {
        [[=Tooltip{"Texture asset UUID"}, =LookupName{am::AssetType::Texture}]]
        boost::uuids::uuid textureUuid{boost::uuids::nil_uuid()};

        [[=NonSerialized{}]]
        gfx::TextureHandle runtimeTextureHandle{gfx::TextureHandle::invalid()};

        [[=Tooltip{"Quad tint color"}]]
        glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};

        [[=Tooltip{"Texture UV bounds (uMin, vMin, uMax, vMax)"}]]
        glm::vec4 uvRect{0.0f, 0.0f, 1.0f, 1.0f};

        [[=Tooltip{"Whether the UI element is visible"}]]
        bool visible{true};

        [[=NonSerialized{}, =HidenInInspector{}]]
        bool isDirty{true};

        UIImageComponent() = default;
        explicit UIImageComponent(glm::vec4 col) : color(col) {}
        UIImageComponent(boost::uuids::uuid texId, glm::vec4 col = glm::vec4(1.0f))
            : textureUuid(texId), color(col) {}
        UIImageComponent(gfx::TextureHandle handle, glm::vec4 col = glm::vec4(1.0f))
            : runtimeTextureHandle(handle), color(col) {}
    };
}
