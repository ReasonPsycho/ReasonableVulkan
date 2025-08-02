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

        virtual void OnComponentAdded(Entity entity);
        virtual void OnComponentRemoved(Entity entit);

        std::string name;
        Signature signature;
    };
}
#endif //SYSTEMBASE_H
