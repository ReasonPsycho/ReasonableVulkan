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
        void InsertData(std::uint32_t entity, T component);
        void RemoveData(std::uint32_t entity);
        T& GetData(std::uint32_t entity);
        bool HasData(std::uint32_t entity);
        void SetComponentActive<T>(Entity entity, bool active);
        bool IsComponentActive<T>(Entity entity);

    private:
        std::array<T, MAX_ENTITIES> componentArray;
        std::bitset<MAX_ENTITIES> activeComponents;
        std::unordered_map<Entity, std::size_t> entityToIndexMap;
        std::unordered_map<std::size_t, Entity> indexToEntityMap;
        std::size_t size = 0;
    };


};



#endif //COMPONENTARRAY_H
