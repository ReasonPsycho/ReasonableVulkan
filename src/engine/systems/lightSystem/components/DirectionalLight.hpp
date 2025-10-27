//
// Created by redkc on 24/10/2025.
//

#ifndef REASONABLEVULKAN_DIRECTIONALLIGHT_HPP
#define REASONABLEVULKAN_DIRECTIONALLIGHT_HPP
#include "LightBase.hpp"


namespace engine::ecs
{
    struct DirectionalLight : public LightBase
    {
        float constant;
        float linear;
        float quadratic;
        float radius;
        float innerCutoff;
        float outerCutoff;

        DirectionalLight()
            : LightBase()
            , constant(1.0f)
            , linear(0.09f)
            , quadratic(0.032f)
            , radius(50.0f)
            , innerCutoff(12.5f)
            , outerCutoff(17.5f)
        {
            type = LightType::Directional;
        }

        void ShowImGui(Scene* scene,Component* component) const override;

        void SerializeToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override;
        void DeserializeFromJson(const rapidjson::Value& obj) override;
    };


}
#endif //REASONABLEVULKAN_DIRECTIONALLIGHT_HPP