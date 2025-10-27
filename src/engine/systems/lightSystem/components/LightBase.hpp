//
// Created by redkc on 24/10/2025.
//

#ifndef REASONABLEVULKAN_LIGHTBASE_HPP
#define REASONABLEVULKAN_LIGHTBASE_HPP
#include <glm/vec3.hpp>
#include "ecs/Component.hpp"

namespace engine::ecs
{

    enum class LightType {
        Directional,
        Point,
        Spot
    };

struct LightBase : public engine::ecs::Component
{
    glm::vec3 color;
    float intensity;
    LightType type;

    LightBase()
        : color(1.0f, 1.0f, 1.0f)
        , intensity(1.0f)
        , type(LightType::Directional)
    {}

    virtual void ShowImGui(Scene* scene,Component* component) const override = 0;

    virtual void SerializeToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override = 0;
    virtual void DeserializeFromJson(const rapidjson::Value& obj) override = 0;
};

}
#endif //REASONABLEVULKAN_LIGHTBASE_HPP