#ifndef REASONABLEVULKAN_ENGINETYPES_HPP
#define REASONABLEVULKAN_ENGINETYPES_HPP

#include <cstdint>
#include <cstddef>
#include <bitset>

namespace engine::ecs {
    class Scene;
    class Entity;

    using EntityID = std::uint32_t;
    using ComponentID = std::uint32_t;
    constexpr std::size_t MAX_COMPONENTS = 32;
    using Signature = std::bitset<MAX_COMPONENTS>;
    constexpr EntityID MAX_ENTITIES = 100000;
    constexpr EntityID MAX_COMPONENTS_ARRAY = 100000;

    struct SceneEntityPayload {
        EntityID entity{MAX_ENTITIES};
        Scene* scene{nullptr};
        uint32_t count{1};
        EntityID entities[128]{MAX_ENTITIES};
    };
}

#endif // REASONABLEVULKAN_ENGINETYPES_HPP
