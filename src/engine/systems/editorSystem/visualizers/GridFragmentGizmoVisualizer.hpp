#pragma once

#include "../ComponentGizmoRegistry.hpp"
#include "systems/pathfindingSystem/componets/GridFragment.hpp"

namespace engine::ecs
{
    template <>
    struct ComponentGizmoVisualizer<GridFragment>
    {
        static constexpr std::string_view DisplayName = "Grid Fragments";
        static constexpr bool DefaultEnabled = true;

        static void Draw(const GridFragment& fragment, const glm::mat4& worldTransform, const GizmoContext& ctx);
        static bool HitTest(const GridFragment& fragment, const glm::mat4& worldTransform, const GizmoContext& ctx, float radius = 14.0f);
        static bool GetScreenPosition(const GridFragment& fragment, const glm::mat4& worldTransform, const GizmoContext& ctx, ImVec2& outScreenPos);
    };
}
