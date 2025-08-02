//
// Created by redkc on 23/02/2024.
//
#pragma once
#include "System.h"

#include <algorithm>


using namespace engine::ecs;

template <typename Derived, typename ... Components>
void System<Derived, Components...>::OnComponentAdded(Entity entity)
{
        if (std::find(entities.begin(), entities.end(), entity) == entities.end()) {
            entities.push_back(entity);
            static_cast<Derived*>(this)->OnAddEntity(entity);
        }

}

template <typename Derived, typename ... Components>
void System<Derived, Components...>::OnComponentRemoved(Entity entity)
{
        auto it = std::find(entities.begin(), entities.end(), entity);
        if (it != entities.end()) {
            static_cast<Derived*>(this)->OnRemoveEntity(entity);
            entities.erase(it);
        }

}