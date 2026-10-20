#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <boost/uuid/uuid.hpp>
#include <string>
#include "ecs/Types.h"

namespace engine::events {

    struct HUDClickEvent {
        ecs::Entity entity{0};
        glm::vec2 mousePosition{0.0f, 0.0f};
        int mouseButton{0};
    };

    struct EntityCollisionEvent {
        ecs::Entity entityA{0};
        ecs::Entity entityB{0};
        glm::vec3 hitPoint{0.0f, 0.0f, 0.0f};
        glm::vec3 normal{0.0f, 1.0f, 0.0f};
    };

    struct EntityCreatedEvent {
        ecs::Entity entity{0};
    };

    struct EntityDestroyedEvent {
        ecs::Entity entity{0};
    };

    struct SceneLoadedEvent {
        boost::uuids::uuid sceneId{};
        std::string sceneName{};
    };

    struct WindowResizeEvent {
        int width{0};
        int height{0};
    };

} // namespace engine::events
