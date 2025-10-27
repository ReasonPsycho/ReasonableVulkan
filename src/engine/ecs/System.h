//
// Created by redkc on 23/02/2024.
//

#ifndef REASONABLEGL_SYSTEM_H
#define REASONABLEGL_SYSTEM_H

#include <algorithm>
#include <vector>
#include <string>
#include <boost/core/demangle.hpp>

#include "componentArrays/ComponentType.h"
#include "SystemBase.h"
#include "Types.h"

namespace engine::ecs
{
    class Scene;

    template <typename Derived, typename... Components>
    class System : public SystemBase
    {
    public:

        explicit System(Scene* scene) : scene(scene)
        {
            registeredComponentTypes = {std::type_index(typeid(Components))...};
            name = boost::core::demangle(typeid(Derived).name());
        }

        virtual ~System() = default;

        virtual void Update(float deltaTime) = 0;

        void AddComponent(ComponentID entity, std::type_index type) override
        {

            // Prob could add them to a list now needed for now
            OnComponentAdded(entity, type);
        }

        void RemoveComponent(ComponentID component, std::type_index type) override
        {
            // Prob could add them to a list now needed for now
            OnEntityRemoved(component, type);
        }

        void SerializeToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override {
            // Store system name
            rapidjson::Value nameVal;
            nameVal.SetString(name.c_str(), allocator);
            obj.AddMember("name", nameVal, allocator);

            // Allow derived systems to serialize additional data
            rapidjson::Value extraData(rapidjson::kObjectType);
            static_cast<const Derived*>(this)->SerializeExtraData(extraData, allocator);
            obj.AddMember("extraData", extraData, allocator);
        }

        void DeserializeFromJson(const rapidjson::Value& obj) override {

            // Load extra data from derived system
            if (obj.HasMember("extraData") && obj["extraData"].IsObject()) {
                static_cast<Derived*>(this)->DeserializeExtraData(obj["extraData"]);
            }
        }

    protected:
        Scene* scene;
        virtual void OnComponentAdded(ComponentID componentID, std::type_index type) = 0;
        virtual void OnEntityRemoved(ComponentID componentID, std::type_index type)  = 0;

        virtual void SerializeExtraData(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const {
            // Default implementation does nothing
        }

        virtual void DeserializeExtraData(const rapidjson::Value& obj) {
            // Default implementation does nothing
        }

    };
}


#endif //REASONABLEGL_SYSTEM_H
