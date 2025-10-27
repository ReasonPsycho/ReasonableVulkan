//
// Created by redkc on 24/10/2025.
//
#pragma once

#ifndef REASONABLEVULKAN_POINTLIGHT_HPP
#include "LightBase.hpp"


namespace engine::ecs
{
    struct PointLight : public LightBase
    {
        float constant;
        float linear;
        float quadratic;
        float radius;

        PointLight()
            : LightBase()
            , constant(1.0f)
            , linear(0.09f)
            , quadratic(0.032f)
            , radius(50.0f)
        {
            type = LightType::Point;
        }

        void ShowImGui(Scene* scene,Component* component) const override;

        void SerializeToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override;
        void DeserializeFromJson(const rapidjson::Value& obj) override;

    };

}

#endif //REASONABLEVULKAN_LIGHTBASE_HPP