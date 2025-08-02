//
// Created by redkc on 02/08/2025.
//

#ifndef COMPONENTTYPE_H
#define COMPONENTTYPE_H

namespace engine::ecs
{
    using ComponentTypeID = std::size_t;

    inline ComponentTypeID GetUniqueComponentTypeID()
    {
        static ComponentTypeID lastID = 0u;
        return lastID++;
    }

    template<typename T>
    ComponentTypeID GetComponentTypeID()
    {
        static ComponentTypeID typeID = GetUniqueComponentTypeID();
        return typeID;
    }
}
#endif //COMPONENTTYPE_H
