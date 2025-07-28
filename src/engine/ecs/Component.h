//
// Created by redkc on 22/02/2024.
//


#ifndef REASONABLEGL_COMPONENT_H
#define REASONABLEGL_COMPONENT_H

#include "tracy/Tracy.hpp"



    namespace engine::ecs
    {
        class Entity;

        /**
         * Abstract base class for a Component.
         * A Component can be attached to an Entity to give it specific properties.
         *
         * Since we deal with circular declaration add only component include to subclass in .h class and then add include to entity in .cpp file. See Render class.
         */
        class Component {
        public:
            Component();
            virtual ~Component() = default;

            std::string name;
            void setEntity(Entity* newParentEntity);
            Entity *getEntity();

            void Update();
            void setIsDirty(bool dirtValue);
            bool getIsDirty();
            void Remove();

            Entity *parentEntity = nullptr;
        protected:
            virtual void UpdateImpl() {};


        private:
            bool isDirty = true;
        };
    }

#endif //REASONABLEGL_COMPONENT_H
