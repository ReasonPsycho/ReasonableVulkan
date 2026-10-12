#include "LightGizmoVisualizer.hpp"

#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

#include "systems/renderingSystem/componets/LightComponent.hpp"


namespace
{
    bool WorldToScreen(const glm::vec3& worldPos, const glm::mat4& viewProj, const ImVec2& vpPos, const ImVec2& vpSize, ImVec2& outScreenPos)
    {
        glm::vec4 clip = viewProj * glm::vec4(worldPos, 1.0f);
        if (clip.w <= 0.0001f) return false;

        glm::vec3 ndc = glm::vec3(clip) / clip.w;
        outScreenPos.x = vpPos.x + (ndc.x + 1.0f) * 0.5f * vpSize.x;
        outScreenPos.y = vpPos.y + (1.0f - ndc.y) * 0.5f * vpSize.y;
        return true;
    }

    ImU32 GetGizmoColor(const glm::vec3& color, float alpha)
    {
        float r = glm::clamp(color.r, 0.0f, 1.0f);
        float g = glm::clamp(color.g, 0.0f, 1.0f);
        float b = glm::clamp(color.b, 0.0f, 1.0f);
        float a = glm::clamp(alpha, 0.0f, 1.0f);
        return IM_COL32((int)(r * 255.0f), (int)(g * 255.0f), (int)(b * 255.0f), (int)(a * 255.0f));
    }

    void DrawWireCircle3D(ImDrawList* drawList, const glm::mat4& viewProj, const ImVec2& vpPos, const ImVec2& vpSize,
                          const glm::vec3& center, const glm::vec3& u, const glm::vec3& v, float radius, ImU32 color, float thickness, int segments = 36)
    {
        ImVec2 prevScreen;
        bool hasPrev = false;
        constexpr float TWO_PI = 6.28318530717958647692f;
        for (int i = 0; i <= segments; ++i)
        {
            float angle = (float)i * (TWO_PI / (float)segments);
            glm::vec3 pt = center + (u * std::cos(angle) + v * std::sin(angle)) * radius;
            ImVec2 screenPt;
            if (WorldToScreen(pt, viewProj, vpPos, vpSize, screenPt))
            {
                if (hasPrev)
                {
                    drawList->AddLine(prevScreen, screenPt, color, thickness);
                }
                prevScreen = screenPt;
                hasPrev = true;
            }
            else
            {
                hasPrev = false;
            }
        }
    }

    void DrawLine3D(ImDrawList* drawList, const glm::mat4& viewProj, const ImVec2& vpPos, const ImVec2& vpSize,
                    const glm::vec3& start, const glm::vec3& end, ImU32 color, float thickness)
    {
        ImVec2 p0, p1;
        if (WorldToScreen(start, viewProj, vpPos, vpSize, p0) && WorldToScreen(end, viewProj, vpPos, vpSize, p1))
        {
            drawList->AddLine(p0, p1, color, thickness);
        }
    }

    void DrawArrow3D(ImDrawList* drawList, const glm::mat4& viewProj, const ImVec2& vpPos, const ImVec2& vpSize,
                     const glm::vec3& start, const glm::vec3& dir, float length, const glm::vec3& right, const glm::vec3& up, ImU32 color, float thickness)
    {
        glm::vec3 end = start + dir * length;
        DrawLine3D(drawList, viewProj, vpPos, vpSize, start, end, color, thickness);

        float headLen = length * 0.22f;
        float headRadius = length * 0.08f;
        glm::vec3 headBase = end - dir * headLen;

        DrawLine3D(drawList, viewProj, vpPos, vpSize, end, headBase + right * headRadius, color, thickness);
        DrawLine3D(drawList, viewProj, vpPos, vpSize, end, headBase - right * headRadius, color, thickness);
        DrawLine3D(drawList, viewProj, vpPos, vpSize, end, headBase + up * headRadius, color, thickness);
        DrawLine3D(drawList, viewProj, vpPos, vpSize, end, headBase - up * headRadius, color, thickness);
    }

    void DrawBillboardLightIcon(ImDrawList* drawList, const ImVec2& screenPos, engine::ecs::LightComponent::Type type, ImU32 lightColorU32, bool isSelected, bool isHovered)
    {
        float iconRadius = isSelected ? 14.0f : (isHovered ? 13.0f : 12.0f);

        ImU32 bgCol = isSelected ? IM_COL32(40, 45, 55, 230) : (isHovered ? IM_COL32(35, 38, 48, 210) : IM_COL32(25, 28, 35, 180));
        drawList->AddCircleFilled(screenPos, iconRadius, bgCol);

        ImU32 ringCol = isSelected ? IM_COL32(255, 255, 255, 255) : (isHovered ? IM_COL32(255, 230, 100, 240) : lightColorU32);
        drawList->AddCircle(screenPos, iconRadius, ringCol, 0, isSelected ? 2.5f : 1.5f);

        switch (type)
        {
            case engine::ecs::LightComponent::Type::Point:
            {
                float centerR = 3.5f;
                drawList->AddCircleFilled(screenPos, centerR, ringCol);
                for (int r = 0; r < 8; ++r)
                {
                    float a = (float)r * (6.2831853f / 8.0f);
                    float cosA = std::cos(a);
                    float sinA = std::sin(a);
                    ImVec2 p0 = ImVec2(screenPos.x + cosA * (centerR + 2.0f), screenPos.y + sinA * (centerR + 2.0f));
                    ImVec2 p1 = ImVec2(screenPos.x + cosA * (iconRadius - 2.5f), screenPos.y + sinA * (iconRadius - 2.5f));
                    drawList->AddLine(p0, p1, ringCol, 1.2f);
                }
                break;
            }
            case engine::ecs::LightComponent::Type::Spot:
            {
                ImVec2 apex = ImVec2(screenPos.x, screenPos.y - 6.0f);
                ImVec2 bLeft = ImVec2(screenPos.x - 5.5f, screenPos.y + 5.0f);
                ImVec2 bRight = ImVec2(screenPos.x + 5.5f, screenPos.y + 5.0f);
                drawList->AddLine(apex, bLeft, ringCol, 1.4f);
                drawList->AddLine(apex, bRight, ringCol, 1.4f);
                drawList->AddLine(bLeft, bRight, ringCol, 1.4f);
                drawList->AddCircleFilled(apex, 2.0f, ringCol);
                break;
            }
            case engine::ecs::LightComponent::Type::Directional:
            {
                float sunR = 4.0f;
                drawList->AddCircleFilled(screenPos, sunR, ringCol);
                for (int r = 0; r < 6; ++r)
                {
                    float a = (float)r * (6.2831853f / 6.0f);
                    ImVec2 p0 = ImVec2(screenPos.x + std::cos(a) * (sunR + 1.5f), screenPos.y + std::sin(a) * (sunR + 1.5f));
                    ImVec2 p1 = ImVec2(screenPos.x + std::cos(a) * (iconRadius - 2.0f), screenPos.y + std::sin(a) * (iconRadius - 2.0f));
                    drawList->AddLine(p0, p1, ringCol, 1.0f);
                }
                break;
            }
        }
    }

    void DrawPointLightGizmo(ImDrawList* drawList, const glm::mat4& viewProj, const ImVec2& vpPos, const ImVec2& vpSize,
                            const glm::mat4& worldTransform, const engine::ecs::PointLightData& data, const glm::vec3& color, bool isSelected)
    {
        glm::vec3 pos = glm::vec3(worldTransform[3]);
        glm::vec3 right = glm::normalize(glm::vec3(worldTransform[0]));
        glm::vec3 up = glm::normalize(glm::vec3(worldTransform[1]));
        glm::vec3 forward = glm::normalize(glm::vec3(worldTransform[2]));

        float radius = data.radius;
        ImU32 col = isSelected ? GetGizmoColor(color, 0.9f) : GetGizmoColor(color, 0.4f);
        float thickness = isSelected ? 1.5f : 1.0f;

        DrawWireCircle3D(drawList, viewProj, vpPos, vpSize, pos, right, up, radius, col, thickness);
        DrawWireCircle3D(drawList, viewProj, vpPos, vpSize, pos, right, forward, radius, col, thickness);
        DrawWireCircle3D(drawList, viewProj, vpPos, vpSize, pos, up, forward, radius, col, thickness);

        float axisLen = isSelected ? (radius * 0.15f) : (radius * 0.1f);
        DrawLine3D(drawList, viewProj, vpPos, vpSize, pos - right * axisLen, pos + right * axisLen, col, thickness);
        DrawLine3D(drawList, viewProj, vpPos, vpSize, pos - up * axisLen, pos + up * axisLen, col, thickness);
        DrawLine3D(drawList, viewProj, vpPos, vpSize, pos - forward * axisLen, pos + forward * axisLen, col, thickness);
    }

    void DrawSpotLightGizmo(ImDrawList* drawList, const glm::mat4& viewProj, const ImVec2& vpPos, const ImVec2& vpSize,
                            const glm::mat4& worldTransform, const engine::ecs::SpotLightData& data, const glm::vec3& color, bool isSelected)
    {
        glm::vec3 pos = glm::vec3(worldTransform[3]);
        glm::vec3 right = glm::normalize(glm::vec3(worldTransform[0]));
        glm::vec3 up = glm::normalize(glm::vec3(worldTransform[1]));
        glm::vec3 forward = glm::normalize(glm::vec3(worldTransform * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));

        float range = data.range;
        float outerAngleRad = glm::radians(data.outerAngle);
        float innerAngleRad = glm::radians(data.innerAngle);

        float outerRadius = range * std::tan(outerAngleRad);
        float innerRadius = range * std::tan(innerAngleRad);

        glm::vec3 baseCenter = pos + forward * range;

        ImU32 outerCol = isSelected ? GetGizmoColor(color, 0.9f) : GetGizmoColor(color, 0.4f);
        ImU32 innerCol = isSelected ? GetGizmoColor(color, 0.45f) : GetGizmoColor(color, 0.2f);
        float thickness = isSelected ? 1.5f : 1.0f;

        DrawWireCircle3D(drawList, viewProj, vpPos, vpSize, baseCenter, right, up, outerRadius, outerCol, thickness);

        DrawLine3D(drawList, viewProj, vpPos, vpSize, pos, baseCenter + right * outerRadius, outerCol, thickness);
        DrawLine3D(drawList, viewProj, vpPos, vpSize, pos, baseCenter - right * outerRadius, outerCol, thickness);
        DrawLine3D(drawList, viewProj, vpPos, vpSize, pos, baseCenter + up * outerRadius, outerCol, thickness);
        DrawLine3D(drawList, viewProj, vpPos, vpSize, pos, baseCenter - up * outerRadius, outerCol, thickness);

        if (data.innerAngle > 0.1f && data.innerAngle < data.outerAngle)
        {
            DrawWireCircle3D(drawList, viewProj, vpPos, vpSize, baseCenter, right, up, innerRadius, innerCol, thickness * 0.8f);
            DrawLine3D(drawList, viewProj, vpPos, vpSize, pos, baseCenter + right * innerRadius, innerCol, thickness * 0.8f);
            DrawLine3D(drawList, viewProj, vpPos, vpSize, pos, baseCenter - right * innerRadius, innerCol, thickness * 0.8f);
            DrawLine3D(drawList, viewProj, vpPos, vpSize, pos, baseCenter + up * innerRadius, innerCol, thickness * 0.8f);
            DrawLine3D(drawList, viewProj, vpPos, vpSize, pos, baseCenter - up * innerRadius, innerCol, thickness * 0.8f);
        }

        DrawLine3D(drawList, viewProj, vpPos, vpSize, pos, baseCenter, innerCol, thickness * 0.8f);
    }

    void DrawDirectionalLightGizmo(ImDrawList* drawList, const glm::mat4& viewProj, const ImVec2& vpPos, const ImVec2& vpSize,
                                  const glm::mat4& worldTransform, const engine::ecs::DirectionalLightData& data, const glm::vec3& color, bool isSelected)
    {
        glm::vec3 pos = glm::vec3(worldTransform[3]);
        glm::vec3 right = glm::normalize(glm::vec3(worldTransform[0]));
        glm::vec3 up = glm::normalize(glm::vec3(worldTransform[1]));
        glm::vec3 forward = glm::normalize(glm::vec3(worldTransform * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));

        float ringRadius = 1.5f;
        float arrowLength = 3.0f;

        ImU32 col = isSelected ? GetGizmoColor(color, 0.9f) : GetGizmoColor(color, 0.4f);
        float thickness = isSelected ? 1.5f : 1.0f;

        DrawWireCircle3D(drawList, viewProj, vpPos, vpSize, pos, right, up, ringRadius, col, thickness);

        DrawArrow3D(drawList, viewProj, vpPos, vpSize, pos, forward, arrowLength, right, up, col, thickness);

        DrawArrow3D(drawList, viewProj, vpPos, vpSize, pos + right * ringRadius, forward, arrowLength, right, up, col, thickness);
        DrawArrow3D(drawList, viewProj, vpPos, vpSize, pos - right * ringRadius, forward, arrowLength, right, up, col, thickness);
        DrawArrow3D(drawList, viewProj, vpPos, vpSize, pos + up * ringRadius, forward, arrowLength, right, up, col, thickness);
        DrawArrow3D(drawList, viewProj, vpPos, vpSize, pos - up * ringRadius, forward, arrowLength, right, up, col, thickness);
    }
}

namespace engine::ecs
{
    struct GizmoContext;

    void ComponentGizmoVisualizer<LightComponent>::Draw(const LightComponent& light, const glm::mat4& worldTransform, const GizmoContext& ctx)
    {
        if (!ctx.drawList) return;

        glm::vec3 pos = glm::vec3(worldTransform[3]);
        ImVec2 screenPos;
        bool visibleOnScreen = WorldToScreen(pos, ctx.viewProj, ctx.viewportPos, ctx.viewportSize, screenPos);

        if (light.showGizmo || ctx.isSelected)
        {
            switch (light.type)
            {
                case LightComponent::Type::Point:
                    if (std::holds_alternative<PointLightData>(light.data))
                    {
                        DrawPointLightGizmo(ctx.drawList, ctx.viewProj, ctx.viewportPos, ctx.viewportSize, worldTransform, std::get<PointLightData>(light.data), light.color, ctx.isSelected);
                    }
                    break;
                case LightComponent::Type::Spot:
                    if (std::holds_alternative<SpotLightData>(light.data))
                    {
                        DrawSpotLightGizmo(ctx.drawList, ctx.viewProj, ctx.viewportPos, ctx.viewportSize, worldTransform, std::get<SpotLightData>(light.data), light.color, ctx.isSelected);
                    }
                    break;
                case LightComponent::Type::Directional:
                    if (std::holds_alternative<DirectionalLightData>(light.data))
                    {
                        DrawDirectionalLightGizmo(ctx.drawList, ctx.viewProj, ctx.viewportPos, ctx.viewportSize, worldTransform, std::get<DirectionalLightData>(light.data), light.color, ctx.isSelected);
                    }
                    break;
            }
        }

        if (visibleOnScreen)
        {
            ImU32 lightColU32 = GetGizmoColor(light.color, 1.0f);
            DrawBillboardLightIcon(ctx.drawList, screenPos, light.type, lightColU32, ctx.isSelected, ctx.isHovered);
        }
    }

    bool ComponentGizmoVisualizer<LightComponent>::GetScreenPosition(const LightComponent& light, const glm::mat4& worldTransform, const GizmoContext& ctx, ImVec2& outScreenPos)
    {
        glm::vec3 pos = glm::vec3(worldTransform[3]);
        return WorldToScreen(pos, ctx.viewProj, ctx.viewportPos, ctx.viewportSize, outScreenPos);
    }

    bool ComponentGizmoVisualizer<LightComponent>::HitTest(const LightComponent& light, const glm::mat4& worldTransform, const GizmoContext& ctx, float radius)
    {
        ImVec2 iconScreenPos;
        if (!GetScreenPosition(light, worldTransform, ctx, iconScreenPos))
        {
            return false;
        }

        float dx = ctx.mousePos.x - iconScreenPos.x;
        float dy = ctx.mousePos.y - iconScreenPos.y;
        float distSq = dx * dx + dy * dy;

        return distSq <= (radius * radius);
    }
}
