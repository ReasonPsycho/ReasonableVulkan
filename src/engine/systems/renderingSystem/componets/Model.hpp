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
    struct Model : public Component
    {
        boost::uuids::uuid modelUuid;
        glm::vec3 boundingBoxMin;
        glm::vec3 boundingBoxMax;

        Model() : modelUuid(boost::uuids::nil_uuid()), boundingBoxMin(0.0f), boundingBoxMax(0.0f) {}
        explicit Model(boost::uuids::uuid id) : modelUuid(id),boundingBoxMin(0.0f),boundingBoxMax(0.0f) {}

        void ShowImGui(Scene* scene,Component* component) const override;

        void SerializeToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override;
        void DeserializeFromJson(const rapidjson::Value& obj) override;
    };
}

#endif //REASONABLEVULKAN_MODEL_HPP