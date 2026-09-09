//
// Created by redkc on 23/02/2024.
//

#ifndef REASONABLEGL_SYSTEM_H
#define REASONABLEGL_SYSTEM_H

#include <algorithm>
#include <vector>
#include <string>
#include <meta>

#include "componentArrays/ComponentType.h"
#include "SystemBase.h"
#include "Types.h"
#include "Reflection.hpp"

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
            name = std::meta::identifier_of(^^Derived);
        }

        virtual ~System() = default;


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

            // Serialize reflected member variables of derived system
            rapidjson::Value extraData(rapidjson::kObjectType);
            SerializeTypeToJson(*static_cast<const Derived*>(this), extraData, allocator);
            obj.AddMember("extraData", extraData, allocator);
        }

        void DeserializeFromJson(const rapidjson::Value& obj) override {
            // Deserialize reflected member variables of derived system
            if (obj.HasMember("extraData") && obj["extraData"].IsObject()) {
                DeserializeTypeFromJson(*static_cast<Derived*>(this), obj["extraData"]);
            }
        }

        bool DrawSettingsImGui(Scene* scenePtr) override {
#ifdef ENABLE_IMGUI
            if constexpr (requires { static_cast<Derived*>(this)->CustomDrawSettingsImGui(scenePtr); }) {
                return static_cast<Derived*>(this)->CustomDrawSettingsImGui(scenePtr);
            } else {
                return DrawComponentFields(*static_cast<Derived*>(this), scenePtr);
            }
#else
            return false;
#endif
        }

    protected:
        [[=NonSerialized{}]]
        Scene* scene;
        virtual void OnComponentAdded(ComponentID componentID, std::type_index type) = 0;
        virtual void OnEntityRemoved(ComponentID componentID, std::type_index type)  = 0;

    };
}


#endif //REASONABLEGL_SYSTEM_H
