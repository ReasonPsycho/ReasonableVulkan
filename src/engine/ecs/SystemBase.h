//
// Created by redkc on 02/08/2025.
//

#ifndef SYSTEMBASE_H
#define SYSTEMBASE_H
#include <rapidjson/document.h>

#include "Types.h"

namespace engine::ecs
{
    class SystemBase {
    public:
        virtual ~SystemBase() = default;
        virtual void Update(float deltaTime) = 0;

        virtual void AddEntity(Entity entity) = 0;
        virtual void RemoveEntity(Entity entity) = 0;

        std::string name;
        Signature signature;

        virtual void SerializeToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const = 0;
        virtual void DeserializeFromJson(const rapidjson::Value& obj) = 0;
    };
}
#endif //SYSTEMBASE_H
