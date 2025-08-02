//
// Created by redkc on 23/02/2024.
//

#ifndef REASONABLEGL_SYSTEM_H
#define REASONABLEGL_SYSTEM_H

#include <vector>
#include <string>
#include <boost/core/demangle.hpp>

#include "ComponentType.h"
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

        void OnComponentAdded(Entity entity) override;
        void OnComponentRemoved(Entity entity)  override;

    protected:
        Scene* scene;
        virtual void OnAddEntity(Entity entity) = 0;
        virtual void OnRemoveEntity(Entity entity) = 0;

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

#include "System.tpp"

#endif //REASONABLEGL_SYSTEM_H
