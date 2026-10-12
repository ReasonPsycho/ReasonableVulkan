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
    struct ThumbnailComponent;
    struct ScriptComponent;
    struct MoveLeftScript;

    class RenderSystem;
    class GizmoSystem;
    class TransformSystem;
    class CollisionSystem;
    class ScriptSystem;

    using EngineComponents = std::tuple<
        TransformComponent,
        MeshComponent,
        RendererComponent,
        CameraComponent,
        LightComponent,
        NameComponent,
        TagComponent,
        ThumbnailComponent,
        ScriptComponent
    >;

    using EngineSystems = std::tuple<
        RenderSystem,
        GizmoSystem,
        TransformSystem,
        CollisionSystem,
        ScriptSystem
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
