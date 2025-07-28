//
// Created by redkc on 22/02/2024.
//
#include "Entity.h"
#include "Component.h"

using namespace engine::ecs;

Entity *Component::getEntity() {  // Function to set the parent entity
    return parentEntity;
}

Component::Component() {}

void Component::setIsDirty(bool dirtValue) {
    isDirty = dirtValue;
}

bool Component::getIsDirty() {
    return isDirty;
}

void Component::setEntity(Entity *newParentEntity) {
parentEntity = newParentEntity;
}


void Component::Update() {
    ZoneTransientN(zoneName,(name).c_str(),true);
    UpdateImpl();
}

void Component::Remove() {
    parentEntity->removeComponentFromMap(this);
}
