//
// Created by redkc on 10/10/2025.
//

#ifndef REASONABLEVULKAN_COMPONENT_HPP
#define REASONABLEVULKAN_COMPONENT_HPP
#include <rapidjson/document.h>

namespace engine::ecs
{
    class Scene;

    struct Component
    {
    public:
        virtual ~Component() = default;

        virtual void ShowImGui(Scene* scene, Component* component) const = 0;

        virtual void SerializeToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const = 0;
        virtual void DeserializeFromJson(const rapidjson::Value& obj) = 0;
    };

}
#endif //REASONABLEVULKAN_COMPONENT_HPP
