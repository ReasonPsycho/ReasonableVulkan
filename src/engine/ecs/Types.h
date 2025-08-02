#ifndef ENTITY_H
#define ENTITY_H
#include <bitset>
#include <cstdint>


namespace engine::ecs
{
    using ComponentTypeID = std::size_t;
    using Entity = std::uint32_t;
    constexpr std::size_t MAX_COMPONENTS = 64; // Or more if needed
    using Signature = std::bitset<MAX_COMPONENTS>;
    const Entity MAX_ENTITIES = 5000;
}

#endif