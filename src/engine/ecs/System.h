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

    protected:
        Scene* scene;
        virtual void OnEntityAdded(Entity entity) = 0;
        virtual void OnEntityRemoved(Entity entity)  = 0;

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
