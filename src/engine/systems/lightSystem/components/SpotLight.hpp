//
// Created by redkc on 24/10/2025.
//

#ifndef REASONABLEVULKAN_SPOTLIGHT_HPP
#define REASONABLEVULKAN_SPOTLIGHT_HPP
#include <glm/vec3.hpp>
#include "LightBase.hpp"

namespace engine::ecs
{
    struct SpotLight : public LightBase
    {
        glm::vec3 direction;

        SpotLight()
            : LightBase()
            , direction(0.0f, -1.0f, 0.0f)  // Points downward by default
        {
            type = LightType::Spot;
        }

        void ShowImGui(Scene* scene,Component* component) const override;

        void SerializeToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override;
        void DeserializeFromJson(const rapidjson::Value& obj) override;
    };

}

#endif //REASONABLEVULKAN_SPOTLIGHT_HPP