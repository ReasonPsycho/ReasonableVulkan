//
// Created by redkc on 18/02/2024.
//
#include "Entity.h"
#include "Scene.h"
#include "tracy/Tracy.hpp"
using namespace engine::ecs;

void Scene::updateScene() {
    ZoneScopedN("Update scene");


    for (auto &&child: children) {
        child->updateSelfAndChild();
    }
}

Entity *Scene::addEntity(std::string name) {
    children.push_back(make_unique<Entity>(this, name));
    return children.back().get();
}

void Scene::removeChild(Entity *child) {
    auto iter = std::find_if(children.begin(), children.end(),
                             [&](const std::unique_ptr<Entity> &e) { return e.get() == child; });
 

    if (iter != children.end()) {
        // Entity was found. Now remove it.
        // unique_ptr will automatically delete the Entity when erased.
        children.erase(iter);
    }
    stopRenderingImgui = true;
}


Entity *Scene::addEntity(Entity *parent, std::string name) {
    return parent->addChild(make_unique<Entity>(this, parent, name));
}

std::vector<std::unique_ptr<Entity>> &Scene::getChildren() {
    return children;
}

Entity *Scene::getChild(unsigned int id) const {
    auto found = std::find_if(children.begin(), children.end(), [id](auto & child){
        return child->uniqueID == id;
    });
    return found == children.end() ? nullptr : found->get();
}

Entity *Scene::getChild(const std::string &name) const {
    auto found = std::find_if(children.begin(), children.end(), [name](auto & child){
        return child->name == name;
    });
    return found == children.end() ? nullptr : found->get();
}

Entity *Scene::getChildR(unsigned int id) const {
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

Entity *Scene::getChildR(const std::string &name) const {
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

