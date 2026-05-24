//
// Created by redkc on 01/10/2025.
//

#ifndef REASONABLEVULKAN_MODEL_HPP
#define REASONABLEVULKAN_MODEL_HPP

#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <glm/vec3.hpp>

#include "ecs/Component.hpp"

namespace engine::ecs
{
    struct RendererComponent : public Component
    {
        boost::uuids::uuid modelUuid;
        boost::uuids::uuid shaderUuid;


        RendererComponent() : modelUuid(boost::uuids::nil_uuid()), shaderUuid(boost::uuids::nil_uuid()){}
        explicit RendererComponent(boost::uuids::uuid modelId, boost::uuids::uuid shaderId = boost::uuids::nil_uuid()) : modelUuid(modelId), shaderUuid(shaderId) {}

        void ShowImGui(Scene* scene,Component* component) const override;

        void SerializeComponentToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override;
        void DeserializeComponentFromJson(const rapidjson::Value& obj) override;
    };
}

#endif //REASONABLEVULKAN_MODEL_HPP