#pragma once

#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <imgui.h>
#include <concepts>
#include <string_view>
#include <typeindex>
#include <unordered_map>
#include <optional>
#include <functional>

#include "ecs/utility/Reflection.hpp"
#include "ecs/Scene.h"
#include "ecs/componentArrays/ComponentType.h"
#include "systems/transformSystem/componets/TransformComponent.hpp"

namespace engine::ecs
{
    struct GizmoContext
    {
        ImDrawList* drawList = nullptr;
        glm::mat4 viewProj{1.0f};
        ImVec2 viewportPos{0.0f, 0.0f};
        ImVec2 viewportSize{0.0f, 0.0f};
        ImVec2 mousePos{0.0f, 0.0f};
        bool isSelected = false;
        bool isHovered = false;
    };

    // Forward declaration of generic component visualizer template
    template <typename ComponentType>
    struct ComponentGizmoVisualizer;

    // C++26 concept to detect if a component type has a specialized gizmo visualizer
    template <typename T>
    concept HasGizmoVisualizer = requires(const T& comp, const glm::mat4& transform, const GizmoContext& ctx, ImVec2& screenPos)
    {
        { ComponentGizmoVisualizer<T>::Draw(comp, transform, ctx) } -> std::same_as<void>;
        { ComponentGizmoVisualizer<T>::HitTest(comp, transform, ctx, 14.0f) } -> std::same_as<bool>;
    };

    template <typename ComponentsTuple = EngineComponents>
    class ComponentGizmoRegistry
    {
    public:
        static bool& GetCategoryEnabled(std::type_index typeIdx)
        {
            static std::unordered_map<std::type_index, bool> s_EnabledMap = []() {
                std::unordered_map<std::type_index, bool> map;
                ForEachType<ComponentsTuple>([&map]<typename T>() {
                    if constexpr (HasGizmoVisualizer<T>) {
                        if constexpr (requires { ComponentGizmoVisualizer<T>::DefaultEnabled; }) {
                            map[std::type_index(typeid(T))] = ComponentGizmoVisualizer<T>::DefaultEnabled;
                        } else {
                            map[std::type_index(typeid(T))] = true;
                        }
                    }
                });
                return map;
            }();
            return s_EnabledMap[typeIdx];
        }

        template <typename T>
        static bool IsGizmoEnabled()
        {
            if constexpr (HasGizmoVisualizer<T>) {
                return GetCategoryEnabled(std::type_index(typeid(T)));
            }
            return false;
        }

        template <typename T>
        static void SetGizmoEnabled(bool enabled)
        {
            if constexpr (HasGizmoVisualizer<T>) {
                GetCategoryEnabled(std::type_index(typeid(T))) = enabled;
            }
        }

        // Uses C++26 reflection over EngineComponents to display toolbar toggles for all components with visualizers
        static void DrawToolbarToggles()
        {
            ForEachType<ComponentsTuple>([]<typename T>() {
                if constexpr (HasGizmoVisualizer<T>) {
                    std::string_view name;
                    if constexpr (requires { ComponentGizmoVisualizer<T>::DisplayName; }) {
                        name = ComponentGizmoVisualizer<T>::DisplayName;
                    } else {
                        name = std::meta::identifier_of(^^T);
                    }

                    bool& enabled = GetCategoryEnabled(std::type_index(typeid(T)));
                    ImGui::Checkbox(name.data(), &enabled);
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Toggle %.*s Gizmos", static_cast<int>(name.size()), name.data());
                    }
                    ImGui::SameLine();
                }
            });
        }

        // Uses C++26 reflection over EngineComponents to automatically draw all active visualizers for entities in the scene
        template <typename IsSelectedPredicate>
        static void DrawAllGizmos(Scene* scene, const GizmoContext& ctx, IsSelectedPredicate&& isSelected)
        {
            if (!scene || !ctx.drawList) return;

            auto transformArray = scene->GetIntegralComponentArray<TransformComponent>();
            if (!transformArray) return;

            ForEachType<ComponentsTuple>([&]<typename T>() {
                if constexpr (HasGizmoVisualizer<T>) {
                    if (!GetCategoryEnabled(std::type_index(typeid(T)))) return;

                    auto compArray = scene->GetComponentArray<T>();
                    if (!compArray) return;

                    for (ComponentID i = 0; i < compArray->GetArraySize(); ++i)
                    {
                        if (!compArray->IsComponentActive(i)) continue;
                        Entity entity = compArray->ComponentIndexToEntity(i);
                        if (!transformArray->HasComponent(entity)) continue;

                        const auto& comp = compArray->GetComponentFromEntity(entity);
                        const auto& transform = transformArray->GetComponentFromEntity(entity);

                        GizmoContext entityCtx = ctx;
                        entityCtx.isSelected = isSelected(entity);
                        entityCtx.isHovered = ComponentGizmoVisualizer<T>::HitTest(comp, transform.globalMatrix, ctx, 14.0f);

                        ComponentGizmoVisualizer<T>::Draw(comp, transform.globalMatrix, entityCtx);
                    }
                }
            });
        }

        // Uses C++26 reflection over EngineComponents to hit-test gizmo icons across all components
        static std::optional<Entity> HitTestAllGizmos(Scene* scene, const GizmoContext& ctx, float hitRadius = 16.0f)
        {
            if (!scene) return std::nullopt;

            auto transformArray = scene->GetIntegralComponentArray<TransformComponent>();
            if (!transformArray) return std::nullopt;

            std::optional<Entity> hitEntity = std::nullopt;

            ForEachType<ComponentsTuple>([&]<typename T>() {
                if (hitEntity.has_value()) return;

                if constexpr (HasGizmoVisualizer<T>) {
                    if (!GetCategoryEnabled(std::type_index(typeid(T)))) return;

                    auto compArray = scene->GetComponentArray<T>();
                    if (!compArray) return;

                    for (ComponentID i = 0; i < compArray->GetArraySize(); ++i)
                    {
                        if (!compArray->IsComponentActive(i)) continue;
                        Entity entity = compArray->ComponentIndexToEntity(i);
                        if (!transformArray->HasComponent(entity)) continue;

                        const auto& comp = compArray->GetComponentFromEntity(entity);
                        const auto& transform = transformArray->GetComponentFromEntity(entity);

                        if (ComponentGizmoVisualizer<T>::HitTest(comp, transform.globalMatrix, ctx, hitRadius))
                        {
                            hitEntity = entity;
                            return;
                        }
                    }
                }
            });

            return hitEntity;
        }
    };
}
