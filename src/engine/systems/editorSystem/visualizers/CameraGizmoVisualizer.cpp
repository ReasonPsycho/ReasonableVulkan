#include "CameraGizmoVisualizer.hpp"

#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <algorithm>

#include "systems/renderingSystem/componets/CameraComponent.hpp"

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

    void DrawLine3D(ImDrawList* drawList, const glm::mat4& viewProj, const ImVec2& vpPos, const ImVec2& vpSize,
                    const glm::vec3& start, const glm::vec3& end, ImU32 color, float thickness)
    {
        ImVec2 p0, p1;
        if (WorldToScreen(start, viewProj, vpPos, vpSize, p0) && WorldToScreen(end, viewProj, vpPos, vpSize, p1))
        {
            drawList->AddLine(p0, p1, color, thickness);
        }
    }

    void DrawBillboardCameraIcon(ImDrawList* drawList, const ImVec2& screenPos, ImU32 camColorU32, bool isSelected, bool isHovered)
    {
        float iconRadius = isSelected ? 14.0f : (isHovered ? 13.0f : 12.0f);

        ImU32 bgCol = isSelected ? IM_COL32(30, 42, 56, 230) : (isHovered ? IM_COL32(25, 35, 48, 210) : IM_COL32(18, 26, 36, 180));
        drawList->AddCircleFilled(screenPos, iconRadius, bgCol);

        ImU32 ringCol = isSelected ? IM_COL32(255, 255, 255, 255) : (isHovered ? IM_COL32(255, 230, 100, 240) : camColorU32);
        drawList->AddCircle(screenPos, iconRadius, ringCol, 0, isSelected ? 2.5f : 1.5f);

        // Camera body rectangle
        ImVec2 bodyMin(screenPos.x - 5.5f, screenPos.y - 3.0f);
        ImVec2 bodyMax(screenPos.x + 1.5f, screenPos.y + 4.5f);
        drawList->AddRectFilled(bodyMin, bodyMax, ringCol, 1.5f);

        // Lens cone pointing forward (right)
        ImVec2 p0(screenPos.x + 1.5f, screenPos.y - 1.0f);
        ImVec2 p1(screenPos.x + 6.0f, screenPos.y - 3.5f);
        ImVec2 p2(screenPos.x + 6.0f, screenPos.y + 4.5f);
        ImVec2 p3(screenPos.x + 1.5f, screenPos.y + 2.0f);
        drawList->AddQuadFilled(p0, p1, p2, p3, ringCol);

        // Film reels on top
        drawList->AddCircleFilled(ImVec2(screenPos.x - 3.2f, screenPos.y - 4.2f), 1.6f, ringCol);
        drawList->AddCircleFilled(ImVec2(screenPos.x + 0.2f, screenPos.y - 4.2f), 1.6f, ringCol);
    }

    void DrawCameraFrustumGizmo(ImDrawList* drawList, const glm::mat4& viewProj, const ImVec2& vpPos, const ImVec2& vpSize,
                                const glm::mat4& worldTransform, const engine::ecs::CameraComponent& camera, bool isSelected)
    {
        glm::vec3 pos = glm::vec3(worldTransform[3]);
        glm::vec3 right = glm::normalize(glm::vec3(worldTransform[0]));
        glm::vec3 up = glm::normalize(glm::vec3(worldTransform[1]));
        glm::vec3 forward = glm::normalize(glm::vec3(worldTransform * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));

        float fov = std::clamp(camera.fov, 1.0f, 179.0f);
        float aspect = camera.aspectRatio > 0.001f ? camera.aspectRatio : 1.7777778f;
        float halfFovRad = glm::radians(fov * 0.5f);
        float tanFov = std::tan(halfFovRad);

        float nearDist = std::max(camera.nearPlane, 0.1f);
        float hNear = nearDist * tanFov;
        float wNear = hNear * aspect;

        float farDist = std::min(camera.farPlane, std::max(nearDist + 0.5f, 3.5f));
        float hFar = farDist * tanFov;
        float wFar = hFar * aspect;

        // Near plane corners
        glm::vec3 nearCenter = pos + forward * nearDist;
        glm::vec3 nTL = nearCenter - right * wNear + up * hNear;
        glm::vec3 nTR = nearCenter + right * wNear + up * hNear;
        glm::vec3 nBR = nearCenter + right * wNear - up * hNear;
        glm::vec3 nBL = nearCenter - right * wNear - up * hNear;

        // Far preview plane corners
        glm::vec3 farCenter = pos + forward * farDist;
        glm::vec3 fTL = farCenter - right * wFar + up * hFar;
        glm::vec3 fTR = farCenter + right * wFar + up * hFar;
        glm::vec3 fBR = farCenter + right * wFar - up * hFar;
        glm::vec3 fBL = farCenter - right * wFar - up * hFar;

        ImU32 frustumCol = isSelected ? IM_COL32(110, 215, 255, 230) : IM_COL32(70, 160, 230, 140);
        float thickness = isSelected ? 1.8f : 1.2f;

        // Near plane rectangle
        DrawLine3D(drawList, viewProj, vpPos, vpSize, nTL, nTR, frustumCol, thickness);
        DrawLine3D(drawList, viewProj, vpPos, vpSize, nTR, nBR, frustumCol, thickness);
        DrawLine3D(drawList, viewProj, vpPos, vpSize, nBR, nBL, frustumCol, thickness);
        DrawLine3D(drawList, viewProj, vpPos, vpSize, nBL, nTL, frustumCol, thickness);

        // Far plane rectangle
        DrawLine3D(drawList, viewProj, vpPos, vpSize, fTL, fTR, frustumCol, thickness);
        DrawLine3D(drawList, viewProj, vpPos, vpSize, fTR, fBR, frustumCol, thickness);
        DrawLine3D(drawList, viewProj, vpPos, vpSize, fBR, fBL, frustumCol, thickness);
        DrawLine3D(drawList, viewProj, vpPos, vpSize, fBL, fTL, frustumCol, thickness);

        // Connecting pyramid edges (from origin to near plane and near plane to far plane)
        DrawLine3D(drawList, viewProj, vpPos, vpSize, pos, nTL, frustumCol, thickness);
        DrawLine3D(drawList, viewProj, vpPos, vpSize, pos, nTR, frustumCol, thickness);
        DrawLine3D(drawList, viewProj, vpPos, vpSize, pos, nBR, frustumCol, thickness);
        DrawLine3D(drawList, viewProj, vpPos, vpSize, pos, nBL, frustumCol, thickness);

        DrawLine3D(drawList, viewProj, vpPos, vpSize, nTL, fTL, frustumCol, thickness);
        DrawLine3D(drawList, viewProj, vpPos, vpSize, nTR, fTR, frustumCol, thickness);
        DrawLine3D(drawList, viewProj, vpPos, vpSize, nBR, fBR, frustumCol, thickness);
        DrawLine3D(drawList, viewProj, vpPos, vpSize, nBL, fBL, frustumCol, thickness);

        // "Up" indicator triangle above the near plane
        glm::vec3 topMid = (nTL + nTR) * 0.5f;
        glm::vec3 apex = topMid + up * (hNear * 0.45f);
        glm::vec3 leftBase = topMid - right * (wNear * 0.25f);
        glm::vec3 rightBase = topMid + right * (wNear * 0.25f);
        DrawLine3D(drawList, viewProj, vpPos, vpSize, leftBase, apex, frustumCol, thickness);
        DrawLine3D(drawList, viewProj, vpPos, vpSize, rightBase, apex, frustumCol, thickness);

        // Center line
        ImU32 centerCol = isSelected ? IM_COL32(110, 215, 255, 120) : IM_COL32(70, 160, 230, 70);
        DrawLine3D(drawList, viewProj, vpPos, vpSize, pos, farCenter, centerCol, thickness * 0.8f);
    }
}

namespace engine::ecs
{
    void ComponentGizmoVisualizer<CameraComponent>::Draw(const CameraComponent& camera, const glm::mat4& worldTransform, const GizmoContext& ctx)
    {
        if (!ctx.drawList) return;

        glm::vec3 pos = glm::vec3(worldTransform[3]);
        ImVec2 screenPos;
        bool visibleOnScreen = WorldToScreen(pos, ctx.viewProj, ctx.viewportPos, ctx.viewportSize, screenPos);

        if (camera.showGizmo || ctx.isSelected)
        {
            DrawCameraFrustumGizmo(ctx.drawList, ctx.viewProj, ctx.viewportPos, ctx.viewportSize, worldTransform, camera, ctx.isSelected);
        }

        if (visibleOnScreen)
        {
            ImU32 camColU32 = IM_COL32(90, 185, 255, 255);
            DrawBillboardCameraIcon(ctx.drawList, screenPos, camColU32, ctx.isSelected, ctx.isHovered);
        }
    }

    bool ComponentGizmoVisualizer<CameraComponent>::GetScreenPosition(const CameraComponent& camera, const glm::mat4& worldTransform, const GizmoContext& ctx, ImVec2& outScreenPos)
    {
        glm::vec3 pos = glm::vec3(worldTransform[3]);
        return WorldToScreen(pos, ctx.viewProj, ctx.viewportPos, ctx.viewportSize, outScreenPos);
    }

    bool ComponentGizmoVisualizer<CameraComponent>::HitTest(const CameraComponent& camera, const glm::mat4& worldTransform, const GizmoContext& ctx, float radius)
    {
        ImVec2 iconScreenPos;
        if (!GetScreenPosition(camera, worldTransform, ctx, iconScreenPos))
        {
            return false;
        }

        float dx = ctx.mousePos.x - iconScreenPos.x;
        float dy = ctx.mousePos.y - iconScreenPos.y;
        float distSq = dx * dx + dy * dy;

        return distSq <= (radius * radius);
    }
}
