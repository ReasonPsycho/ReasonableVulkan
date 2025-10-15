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
        std::vector<Entity> entities;

        explicit System(Scene* scene) : scene(scene)
        {
            signature = GenerateSignature<Components...>();
            name = boost::core::demangle(typeid(Derived).name());
        }

        virtual ~System() = default;

        void AddEntity(Entity entity) override
        {
            entities.push_back(entity);
            OnEntityAdded(entity);
        }

        void RemoveEntity(Entity entity) override
        {
            auto it = std::find(entities.begin(), entities.end(), entity);
            if (it != entities.end()) {
                entities.erase(it);
                OnEntityRemoved(entity);
            }
        }

        void SerializeToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override {
            // Store system name
            rapidjson::Value nameVal;
            nameVal.SetString(name.c_str(), allocator);
            obj.AddMember("name", nameVal, allocator);

            // Store signature
            std::string sigStr = signature.to_string();
            rapidjson::Value sigVal;
            sigVal.SetString(sigStr.c_str(), allocator);
            obj.AddMember("signature", sigVal, allocator);

            // Store entities
            rapidjson::Value entitiesArray(rapidjson::kArrayType);
            for (Entity entity : entities) {
                entitiesArray.PushBack(static_cast<uint64_t>(entity), allocator);
            }
            obj.AddMember("entities", entitiesArray, allocator);

            // Allow derived systems to serialize additional data
            rapidjson::Value extraData(rapidjson::kObjectType);
            static_cast<const Derived*>(this)->SerializeExtraData(extraData, allocator);
            obj.AddMember("extraData", extraData, allocator);
        }

        void DeserializeFromJson(const rapidjson::Value& obj) override {
            // Clear existing entities
            entities.clear();

            // Load entities
            if (obj.HasMember("entities") && obj["entities"].IsArray()) {
                const auto& entitiesArray = obj["entities"];
                for (rapidjson::SizeType i = 0; i < entitiesArray.Size(); i++) {
                    Entity entity = static_cast<Entity>(entitiesArray[i].GetUint64());
                    entities.push_back(entity);
                    OnEntityAdded(entity);
                }
            }

            // Load extra data from derived system
            if (obj.HasMember("extraData") && obj["extraData"].IsObject()) {
                static_cast<Derived*>(this)->DeserializeExtraData(obj["extraData"]);
            }
        }

    protected:
        Scene* scene;
        virtual void OnEntityAdded(Entity entity) = 0;
        virtual void OnEntityRemoved(Entity entity)  = 0;

        virtual void SerializeExtraData(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const {
            // Default implementation does nothing
        }

        virtual void DeserializeExtraData(const rapidjson::Value& obj) {
            // Default implementation does nothing
        }

    private:
        template <typename... Ts>
        static Signature GenerateSignature()
        {
            Signature sig;
            (sig.set(GetComponentTypeID<Ts>()), ...); // Fold expression
            return sig;
        }
    };
}


#endif //REASONABLEGL_SYSTEM_H
