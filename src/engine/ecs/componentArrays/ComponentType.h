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
    struct RendererComponent;
    struct CameraComponent;
    struct LightComponent;

    class RenderSystem;
    class GizmoSystem;
    class TransformSystem;
    class CollisionSystem;
#ifdef EDITOR_ENABLED
    class EditorSystem;
#endif

    using EngineComponents = std::tuple<
        TransformComponent,
        RendererComponent,
        CameraComponent,
        LightComponent
    >;

    using EngineSystems = std::tuple<
        RenderSystem,
        GizmoSystem,
        TransformSystem,
        CollisionSystem
#ifdef EDITOR_ENABLED
        , EditorSystem
#endif
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
