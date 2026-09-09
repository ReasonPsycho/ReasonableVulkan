
#ifndef REASONABLEVULKAN_LIGHT_HPP
#define REASONABLEVULKAN_LIGHT_HPP
#include <glm/vec3.hpp>
#include <variant>

#include "ecs/Component.hpp"

namespace engine::ecs
{
    struct PointLightData {
        [[=Range{0.1f, 100.0f, 0.1f}, =Tooltip{"Point light radius"}]]
        float radius = 10.0f;

        [[=Range{0.0f, 5.0f, 0.1f}, =Tooltip{"Point light falloff"}]]
        float falloff = 1.0f;

        [[=Range{0.0001f, 0.1f, 0.001f}, =Tooltip{"Shadow bias"}]]
        float shadowBias = 0.005f;

        [[=Range{0.0f, 1.0f, 0.05f}, =Tooltip{"Shadow strength"}]]
        float shadowStrength = 1.0f;
    };

    struct SpotLightData {
        [[=Range{0.0f, 90.0f, 0.5f}, =Tooltip{"Spot inner angle in degrees"}]]
        float innerAngle = 25.0f;

        [[=Range{0.0f, 90.0f, 0.5f}, =Tooltip{"Spot outer angle in degrees"}]]
        float outerAngle = 45.0f;

        [[=Range{0.1f, 200.0f, 0.5f}, =Tooltip{"Spot range"}]]
        float range = 50.0f;

        [[=Range{0.0001f, 0.1f, 0.001f}, =Tooltip{"Shadow bias"}]]
        float shadowBias = 0.005f;

        [[=Range{0.0f, 1.0f, 0.05f}, =Tooltip{"Shadow strength"}]]
        float shadowStrength = 1.0f;
    };

    struct DirectionalLightData {
        [[=Range{0.0001f, 0.1f, 0.001f}, =Tooltip{"Shadow bias"}]]
        float shadowBias = 0.005f;

        [[=Range{0.0f, 1.0f, 0.05f}, =Tooltip{"Shadow strength"}]]
        float shadowStrength = 1.0f;
    };

    struct LightComponent : public Component
    {
        enum class Type { Directional, Point, Spot };

        [[=Tooltip{"Light type"}]]
        Type type{Type::Point};

        [[=Color{}, =Tooltip{"Light color"}]]
        glm::vec3 color{1.0f, 1.0f, 1.0f};

        [[=Range{0.0f, 10.0f, 0.1f}, =Tooltip{"Light intensity"}]]
        float intensity{1.0f};

        [[=Tooltip{"Enable shadows"}]]
        bool hasShadow{false};

        [[=Tooltip{"Type-specific light settings"}]]
        std::variant<DirectionalLightData, PointLightData, SpotLightData> data{PointLightData{}};

        Type getType() const { return type; }
        void setType(Type t) {
            type = t;
            switch (t) {
                case Type::Point:
                    if (!std::holds_alternative<PointLightData>(data)) data = PointLightData{};
                    break;
                case Type::Spot:
                    if (!std::holds_alternative<SpotLightData>(data)) data = SpotLightData{};
                    break;
                case Type::Directional:
                    if (!std::holds_alternative<DirectionalLightData>(data)) data = DirectionalLightData{};
                    break;
            }
        }

        LightComponent() : type(Type::Point), color(1.0f, 1.0f, 1.0f), intensity(1.0f), hasShadow(false), data(PointLightData{}) {}

        explicit LightComponent(Type t, glm::vec3 c, float i, bool hS = false) : type(t), color(c), intensity(i), hasShadow(hS) {
            setType(t);
        }

        void PostDeserialize() {
            setType(type);
        }

        void CustomDrawImGui(Scene* scene);
    };
}

#endif //REASONABLEVULKAN_LIGHT_HPP