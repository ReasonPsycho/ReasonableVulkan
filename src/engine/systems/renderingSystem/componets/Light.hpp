
//
// Created by redkc on 22/12/2025.
//

#ifndef REASONABLEVULKAN_LIGHT_HPP
#define REASONABLEVULKAN_LIGHT_HPP
#include <glm/vec3.hpp>

#include "ecs/Component.hpp"


namespace engine::ecs
{
    struct Light : public Component
    {
        enum class Type { Directional, Point, Spot };

        Type type;
        glm::vec3 color;
        float intensity;

        Light() : type(Type::Point), color(1.0f, 1.0f, 1.0f), intensity(1.0f) {}
        explicit Light(Type t, glm::vec3 c, float i) : type(t), color(c), intensity(i) {}

        void ShowImGui(Scene* scene, Component* component) const override;
        void SerializeToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override;
        void DeserializeFromJson(const rapidjson::Value& obj) override;
    };

    struct PointLight : public Component {
        float radius;
        float falloff;

        PointLight() : radius(10.0f), falloff(1.0f) {}
        explicit PointLight(float r, float f) : radius(r), falloff(f) {}

        void ShowImGui(Scene* scene, Component* component) const override;
        void SerializeToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override;
        void DeserializeFromJson(const rapidjson::Value& obj) override;
    };

    struct SpotLight : public Component {
        float innerAngle;
        float outerAngle;
        float range;

        SpotLight() : innerAngle(25.0f), outerAngle(45.0f), range(50.0f) {}
        explicit SpotLight(float inner, float outer, float r) : innerAngle(inner), outerAngle(outer), range(r) {}

        void ShowImGui(Scene* scene, Component* component) const override;
        void SerializeToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override;
        void DeserializeFromJson(const rapidjson::Value& obj) override;
    };
}

#endif //REASONABLEVULKAN_LIGHT_HPP