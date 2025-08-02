//
// Created by redkc on 02/08/2025.
//

#ifndef COMPONENTARRAY_H
#define COMPONENTARRAY_H
#include "Types.h"
#include <unordered_map>
#include <memory>
#include <cassert>


namespace engine::ecs{


    template<typename T>
    class ComponentArray {
    public:
        void AddComponentToEntity(std::uint32_t entity, T component);
        void RemoveComponentFronEntity(std::uint32_t entity);
        T& GetComponent(std::uint32_t entity);
        bool HasComponent(std::uint32_t entity);
        void SetComponentActive<T>(Entity entity, bool active);
        bool IsComponentActive<T>(Entity entity);
        const std::array<T, MAX_ENTITIES>& GetComponents() const;
    private:
        std::array<T, MAX_ENTITIES> componentArray;
        std::bitset<MAX_ENTITIES> activeComponents;
        std::unordered_map<Entity, std::size_t> entityToIndexMap;
        std::unordered_map<std::size_t, Entity> indexToEntityMap;
        std::size_t size = 0;
    };


};



#endif //COMPONENTARRAY_H
