#pragma once

#include "../ComponentGizmoRegistry.hpp"
#include "systems/renderingSystem/componets/LightComponent.hpp"

namespace engine::ecs
{
    template <>
    struct ComponentGizmoVisualizer<LightComponent>
    {
        static constexpr std::string_view DisplayName = "Lights";
        static constexpr bool DefaultEnabled = true;

        static void Draw(const LightComponent& light, const glm::mat4& worldTransform, const GizmoContext& ctx);
        static bool HitTest(const LightComponent& light, const glm::mat4& worldTransform, const GizmoContext& ctx, float radius = 14.0f);
        static bool GetScreenPosition(const LightComponent& light, const glm::mat4& worldTransform, const GizmoContext& ctx, ImVec2& outScreenPos);
    };
}
