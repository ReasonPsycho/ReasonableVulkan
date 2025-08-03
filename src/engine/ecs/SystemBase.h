//
// Created by redkc on 02/08/2025.
//

#ifndef SYSTEMBASE_H
#define SYSTEMBASE_H
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
    };
}
#endif //SYSTEMBASE_H
