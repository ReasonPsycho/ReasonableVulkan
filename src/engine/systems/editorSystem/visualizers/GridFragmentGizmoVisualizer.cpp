#include "GridFragmentGizmoVisualizer.hpp"
#include "systems/pathfindingSystem/PathfindingSystem.hpp"

#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <algorithm>

namespace
{
    float GetSystemCellSize(const engine::ecs::GizmoContext& ctx)
    {
        if (ctx.scene)
        {
            if (auto pathfindingSystem = ctx.scene->GetSystem<engine::ecs::PathfindingSystem>())
            {
                return pathfindingSystem->GetCellSize();
            }
        }
        return 1.0f;
    }

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

    void DrawBillboardGridIcon(ImDrawList* drawList, const ImVec2& screenPos, ImU32 iconColorU32, bool isSelected, bool isHovered)
    {
        float iconRadius = isSelected ? 14.0f : (isHovered ? 13.0f : 12.0f);

        ImU32 bgCol = isSelected ? IM_COL32(25, 45, 45, 230) : (isHovered ? IM_COL32(20, 40, 40, 210) : IM_COL32(18, 30, 32, 180));
        drawList->AddCircleFilled(screenPos, iconRadius, bgCol);

        ImU32 ringCol = isSelected ? IM_COL32(255, 255, 255, 255) : (isHovered ? IM_COL32(255, 230, 100, 240) : iconColorU32);
        drawList->AddCircle(screenPos, iconRadius, ringCol, 0, isSelected ? 2.5f : 1.5f);

        // Grid icon glyph (2x2 grid / hash pattern)
        float lineThick = isSelected ? 1.5f : 1.2f;
        drawList->AddLine(ImVec2(screenPos.x - 5.0f, screenPos.y - 2.0f), ImVec2(screenPos.x + 5.0f, screenPos.y - 2.0f), ringCol, lineThick);
        drawList->AddLine(ImVec2(screenPos.x - 5.0f, screenPos.y + 2.0f), ImVec2(screenPos.x + 5.0f, screenPos.y + 2.0f), ringCol, lineThick);
        drawList->AddLine(ImVec2(screenPos.x - 2.0f, screenPos.y - 5.0f), ImVec2(screenPos.x - 2.0f, screenPos.y + 5.0f), ringCol, lineThick);
        drawList->AddLine(ImVec2(screenPos.x + 2.0f, screenPos.y - 5.0f), ImVec2(screenPos.x + 2.0f, screenPos.y + 5.0f), ringCol, lineThick);
    }

    void DrawGridFragmentGridGizmo(ImDrawList* drawList, const glm::mat4& viewProj, const ImVec2& vpPos, const ImVec2& vpSize,
                                   const glm::mat4& worldTransform, const engine::ecs::GridFragment& fragment, const engine::ecs::GizmoContext& ctx)
    {
        glm::vec3 pos = glm::vec3(worldTransform[3]);
        float cellSz = GetSystemCellSize(ctx);
        if (cellSz <= 0.001f) cellSz = 1.0f;
        int width = std::max(1, fragment.width);
        int height = std::max(1, fragment.height);

        float totalW = static_cast<float>(width) * cellSz;
        float totalH = static_cast<float>(height) * cellSz;
        float minX = pos.x - totalW * 0.5f;
        float minZ = pos.z - totalH * 0.5f;
        float y = pos.y + 0.01f;

        // Outer boundary corners
        glm::vec3 p0(minX, y, minZ);
        glm::vec3 p1(minX + totalW, y, minZ);
        glm::vec3 p2(minX + totalW, y, minZ + totalH);
        glm::vec3 p3(minX, y, minZ + totalH);

        ImU32 outerCol;
        float outerThickness;
        if (ctx.isSelected) {
            outerCol = IM_COL32(80, 240, 220, 255);
            outerThickness = 2.0f;
        } else if (ctx.isHovered) {
            outerCol = IM_COL32(255, 230, 90, 240);
            outerThickness = 1.8f;
        } else {
            outerCol = IM_COL32(70, 200, 160, 180);
            outerThickness = 1.3f;
        }

        // Outer border
        DrawLine3D(drawList, viewProj, vpPos, vpSize, p0, p1, outerCol, outerThickness);
        DrawLine3D(drawList, viewProj, vpPos, vpSize, p1, p2, outerCol, outerThickness);
        DrawLine3D(drawList, viewProj, vpPos, vpSize, p2, p3, outerCol, outerThickness);
        DrawLine3D(drawList, viewProj, vpPos, vpSize, p3, p0, outerCol, outerThickness);

        // Internal grid lines
        ImU32 innerCol = ctx.isSelected ? IM_COL32(100, 220, 190, 140) : IM_COL32(60, 180, 150, 90);
        float innerThickness = ctx.isSelected ? 1.2f : 1.0f;

        for (int lx = 1; lx < width; ++lx)
        {
            float cx = minX + static_cast<float>(lx) * cellSz;
            DrawLine3D(drawList, viewProj, vpPos, vpSize, glm::vec3(cx, y, minZ), glm::vec3(cx, y, minZ + totalH), innerCol, innerThickness);
        }

        for (int ly = 1; ly < height; ++ly)
        {
            float cz = minZ + static_cast<float>(ly) * cellSz;
            DrawLine3D(drawList, viewProj, vpPos, vpSize, glm::vec3(minX, y, cz), glm::vec3(minX + totalW, y, cz), innerCol, innerThickness);
        }

        // Cell walkability indicators
        ImU32 blockedCol = ctx.isSelected ? IM_COL32(255, 70, 70, 230) : IM_COL32(235, 60, 60, 180);
        float blockedThick = ctx.isSelected ? 1.5f : 1.0f;

        for (int ly = 0; ly < height; ++ly)
        {
            for (int lx = 0; lx < width; ++lx)
            {
                bool walkable = fragment.IsCellWalkable(lx, ly);
                if (!walkable)
                {
                    glm::vec3 c00(minX + static_cast<float>(lx) * cellSz, y, minZ + static_cast<float>(ly) * cellSz);
                    glm::vec3 c11(minX + static_cast<float>(lx + 1) * cellSz, y, minZ + static_cast<float>(ly + 1) * cellSz);
                    glm::vec3 c10(minX + static_cast<float>(lx + 1) * cellSz, y, minZ + static_cast<float>(ly) * cellSz);
                    glm::vec3 c01(minX + static_cast<float>(lx) * cellSz, y, minZ + static_cast<float>(ly + 1) * cellSz);

                    DrawLine3D(drawList, viewProj, vpPos, vpSize, c00, c11, blockedCol, blockedThick);
                    DrawLine3D(drawList, viewProj, vpPos, vpSize, c01, c10, blockedCol, blockedThick);
                }
            }
        }
    }
}

namespace engine::ecs
{
    void ComponentGizmoVisualizer<GridFragment>::Draw(const GridFragment& fragment, const glm::mat4& worldTransform, const GizmoContext& ctx)
    {
        if (!ctx.drawList) return;

        glm::vec3 pos = glm::vec3(worldTransform[3]);
        ImVec2 screenPos;
        bool visibleOnScreen = WorldToScreen(pos, ctx.viewProj, ctx.viewportPos, ctx.viewportSize, screenPos);

        if (fragment.showGizmo || ctx.isSelected)
        {
            DrawGridFragmentGridGizmo(ctx.drawList, ctx.viewProj, ctx.viewportPos, ctx.viewportSize, worldTransform, fragment, ctx);
        }

        if (visibleOnScreen)
        {
            ImU32 iconColU32 = IM_COL32(80, 210, 180, 255);
            DrawBillboardGridIcon(ctx.drawList, screenPos, iconColU32, ctx.isSelected, ctx.isHovered);
        }
    }

    bool ComponentGizmoVisualizer<GridFragment>::GetScreenPosition(const GridFragment& fragment, const glm::mat4& worldTransform, const GizmoContext& ctx, ImVec2& outScreenPos)
    {
        glm::vec3 pos = glm::vec3(worldTransform[3]);
        return WorldToScreen(pos, ctx.viewProj, ctx.viewportPos, ctx.viewportSize, outScreenPos);
    }

    bool ComponentGizmoVisualizer<GridFragment>::HitTest(const GridFragment& fragment, const glm::mat4& worldTransform, const GizmoContext& ctx, float radius)
    {
        ImVec2 iconScreenPos;
        if (!GetScreenPosition(fragment, worldTransform, ctx, iconScreenPos))
        {
            return false;
        }

        float dx = ctx.mousePos.x - iconScreenPos.x;
        float dy = ctx.mousePos.y - iconScreenPos.y;
        float distSq = dx * dx + dy * dy;

        return distSq <= (radius * radius);
    }
}
