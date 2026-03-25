
#ifndef REASONABLEVULKAN_LIGHT_HPP
#define REASONABLEVULKAN_LIGHT_HPP
#include <glm/vec3.hpp>
#include <variant>

#include "ecs/Component.hpp"

namespace engine::ecs
{
    struct PointLightData {
        float radius = 10.0f;
        float falloff = 1.0f;
        float shadowBias = 0.005f;
        float shadowStrength = 1.0f;
    };

    struct SpotLightData {
        float innerAngle = 25.0f;
        float outerAngle = 45.0f;
        float range = 50.0f;
        float shadowBias = 0.005f;
        float shadowStrength = 1.0f;
    };

    struct DirectionalLightData {
        float shadowBias = 0.005f;
        float shadowStrength = 1.0f;
    };

    struct LightComponent : public Component
    {
        enum class Type { Directional, Point, Spot };

        Type type;
        glm::vec3 color;
        float intensity;
        bool hasShadow;
        std::variant<DirectionalLightData, PointLightData, SpotLightData> data;

        LightComponent() : type(Type::Point), color(1.0f, 1.0f, 1.0f), intensity(1.0f), hasShadow(false), data(PointLightData{}) {}

        explicit LightComponent(Type t, glm::vec3 c, float i, bool hS = false) : type(t), color(c), intensity(i), hasShadow(hS) {
            switch (t) {
            case Type::Point: data = PointLightData{}; break;
            case Type::Spot: data = SpotLightData{}; break;
            case Type::Directional: data = DirectionalLightData{}; break;
            }
        }

        void ShowImGui(Scene* scene, Component* component) const override;
        void SerializeComponentToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override;
        void DeserializeComponentFromJson(const rapidjson::Value& obj) override;
    };
}

#endif //REASONABLEVULKAN_LIGHT_HPP