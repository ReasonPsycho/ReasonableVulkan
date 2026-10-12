#pragma once

#include "../ComponentGizmoRegistry.hpp"
#include "systems/renderingSystem/componets/CameraComponent.hpp"

namespace engine::ecs
{
    template <>
    struct ComponentGizmoVisualizer<CameraComponent>
    {
        static constexpr std::string_view DisplayName = "Cameras";
        static constexpr bool DefaultEnabled = true;

        static void Draw(const CameraComponent& camera, const glm::mat4& worldTransform, const GizmoContext& ctx);
        static bool HitTest(const CameraComponent& camera, const glm::mat4& worldTransform, const GizmoContext& ctx, float radius = 14.0f);
        static bool GetScreenPosition(const CameraComponent& camera, const glm::mat4& worldTransform, const GizmoContext& ctx, ImVec2& outScreenPos);
    };
}
