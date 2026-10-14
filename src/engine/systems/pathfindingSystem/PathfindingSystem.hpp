//
// Created by redkc on 14/10/2026.
//

#ifndef REASONABLEVULKAN_PATHFINDINGSYSTEM_HPP
#define REASONABLEVULKAN_PATHFINDINGSYSTEM_HPP
#include <typeindex>

#include "ecs/System.h"
#include "ecs/Types.h"

namespace engine::ecs
{
    struct PathfindingAgent;
    class Scene;

class PathfindingSystem : public System<PathfindingAgent> {


    public:
    explicit PathfindingSystem(Scene* scene) : System(scene) {}
    void Update(float deltaTime) override;

    protected:
    void OnComponentAdded(ComponentID componentID, std::type_index type) override;
    void OnEntityRemoved(ComponentID componentID, std::type_index type) override {}

};

}


#endif //REASONABLEVULKAN_PATHFINDINGSYSTEM_HPP
