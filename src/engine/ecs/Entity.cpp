//
// Created by redkc on 10/01/2024.
//

#include "Entity.h"
#include "tracy/Tracy.hpp"
using namespace engine::ecs;


Entity::Entity(Scene *scene, std::string name) : scene(scene), name(name) {}

Entity::Entity(Scene *scene, Entity *parent, std::string name) : scene(scene),
                                                                 parent(parent), name(name) {}


void Entity::forceUpdateSelfAndChild() {
    if (parent)
        transform.computeModelMatrix(parent->transform.getModelMatrix());
    else
        transform.computeModelMatrix();

    for (auto &pair: components) {
        pair.second->setIsDirty(true);
    }

    for (auto &&child: children) {
        child->forceUpdateSelfAndChild();
    }
}

void Entity::updateSelfAndChild() {
    if (transform.isDirty()) {
        forceUpdateSelfAndChild();
        return;
    } else {
        for (auto &pair: components) {
            pair.second->setIsDirty(false);
        }
    }

    if (updateChildren) {
        for (auto &&child: children) {
            child->updateSelfAndChild();
        }
    }

}

void Entity::removeComponentFromMap(Component * comp) {
    std::type_index typeName = std::type_index(typeid(*comp));
    scene->systemManager.removeComponent(comp);
    components.erase(typeName);
}

Entity *Entity::addChild(std::unique_ptr<Entity> child) {
    child->parent = this;
    children.push_back(std::move(child));
    return children.back().get();
}

void Entity::removeChild(Entity *child) {
    std::erase_if(children, [&](const std::unique_ptr<Entity> &e) { return e.get() == child; });
    removedChild = true;
}

Entity::~Entity() {
    while (!components.empty()) {
        removeComponentFromMap(components.begin()->second.get());
    }
}

void Entity::Destroy() {
    while (!children.empty())
        (*children.begin())->Destroy();
    while (!components.empty())
        removeComponentFromMap(components.begin()->second.get());
    if (parent != nullptr) {
        parent->removeChild(this);
    } else {
        scene->removeChild(this);
    }
}

Entity *Entity::getChild(unsigned int id) const {
    auto found = std::find_if(children.begin(), children.end(), [id](auto & child){
        return child->uniqueID == id;
    });
    return found == children.end() ? nullptr : found->get();
}

Entity *Entity::getChild(const std::string &name) const {
    auto found = std::find_if(children.begin(), children.end(), [name](auto & child){
        return child->name == name;
    });
    return found == children.end() ? nullptr : found->get();
}

Entity *Entity::getChildR(unsigned int id) const {
    Entity * found = getChild(id);
    if (found != nullptr)
        return found;

    for (auto & child : children) {
        found = child->getChildR(id);
        if (found != nullptr)
            return found;
    }
    return nullptr;
}

Entity *Entity::getChildR(const std::string &name) const {
    Entity * found = getChild(name);
    if (found != nullptr)
        return found;

    for (auto & child : children) {
        found = child->getChildR(name);
        if (found != nullptr)
            return found;
    }
    return nullptr;
}

