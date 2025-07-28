//
// Created by redkc on 23/02/2024.
//

#ifndef REASONABLEGL_SYSTEM_H
#define REASONABLEGL_SYSTEM_H
#include <iostream>
#include <typeindex>

namespace engine::ecs
{

    class SystemManager;

    class System {
    public:
        System();
        std::string name;

        virtual ~System() = default;

        //Component system
        virtual void addComponent(void* component) = 0;
        virtual void removeComponent(void* component) = 0;
        virtual void registerComponents() = 0;
        virtual const std::type_index* getComponentTypes() = 0;
        virtual int getNumComponentTypes() = 0;

        //Logic

        void Update();

        SystemManager *getSystemManager();

        unsigned uniqueID;     // Instance variable to store the unique ID for each object
        SystemManager* systemManager;
    protected:
        virtual void UpdateImpl(){};
    };
}
#endif //REASONABLEGL_SYSTEM_H
