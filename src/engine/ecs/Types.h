#ifndef ENTITY_H
#define ENTITY_H
#include <bitset>
#include <cstdint>

namespace engine::ecs
{
    using Entity = std::uint32_t;
    constexpr std::size_t MAX_COMPONENTS = 16; // Or more if needed
    using Signature = std::bitset<MAX_COMPONENTS>;
    const Entity MAX_ENTITIES = 100;
    const Entity MAX_COMPONENTS_ARRAY = 10;
}

#endif