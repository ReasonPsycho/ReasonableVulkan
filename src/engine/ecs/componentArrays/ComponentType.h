//
// Created by redkc on 02/08/2025.
//

#ifndef COMPONENTTYPE_H
#define COMPONENTTYPE_H

#include <cstddef>
#include <limits>
#include <tuple>
#include "ecs/Reflection.hpp"

namespace engine::ecs
{
    using ComponentTypeID = std::size_t;

    struct TransformComponent;
    struct MeshComponent;
    struct RendererComponent;
    struct CameraComponent;
    struct LightComponent;
    struct NameComponent;
    struct TagComponent;

    class RenderSystem;
    class GizmoSystem;
    class TransformSystem;
    class CollisionSystem;

    using EngineComponents = std::tuple<
        TransformComponent,
        MeshComponent,
        RendererComponent,
        CameraComponent,
        LightComponent,
        NameComponent,
        TagComponent
    >;

    using EngineSystems = std::tuple<
        RenderSystem,
        GizmoSystem,
        TransformSystem,
        CollisionSystem
    >;

    template<typename T>
    consteval ComponentTypeID GetComponentTypeID()
    {
        constexpr auto id = IndexInTuple<T, EngineComponents>();
        static_assert(id != std::numeric_limits<std::size_t>::max(), "Component not in EngineComponents list!");
        return id;
    }
}
#endif //COMPONENTTYPE_H
