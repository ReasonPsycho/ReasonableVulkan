//
// Created by redkc on 23/02/2024.
//

#ifndef REASONABLEGL_SYSTEM_H
#define REASONABLEGL_SYSTEM_H

#include <algorithm>
#include <vector>
#include <string>
#include <boost/core/demangle.hpp>

#include "ComponentType.h"
#include "Types.h"

namespace engine::ecs
{
    template <typename Derived, typename... Components>
    class System
    {
    public:
        std::string name;
        std::vector<Entity> entities;
        Signature signature;

        System()
        {
            signature = GenerateSignature<Components...>();
            name = boost::core::demangle(typeid(Derived).name());
        }

        virtual ~System() = default;

        virtual void Update(float deltaTime) = 0;

        void OnComponentAdded(Entity entity, const Signature& entitySignature);
        void OnComponentRemoved(Entity entity, const Signature& entitySignature);

    protected:
        virtual void AddEntity(Entity entity) = 0;
        virtual void RemoveEntity(Entity entity) = 0;

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
