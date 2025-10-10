//
// Created by redkc on 09/10/2025.
//

#ifndef REASONABLEVULKAN_EDITORSYSTEM_HPP
#define REASONABLEVULKAN_EDITORSYSTEM_HPP
#include <unordered_map>

#include "ecs/System.h"

namespace engine::ecs
{
    class Scene;
    class EditorSystem :  public System<EditorSystem>
    {
    public:
        EditorSystem(Scene* scene) : System(scene) {}
        void Update(float deltaTime) override;

        void SetEntityName(Entity entity, const std::string& name);
        std::string_view GetEntityName(Entity entity) const;
    protected:
        void OnEntityAdded(Entity entity) override {}
        void OnEntityRemoved(Entity entity) override {}

    private:
        std::unordered_map<Entity, std::string> named_entities;

        void ShowSceneGraph();
        void ShowEntity(Entity entity);
    };
}

#endif //REASONABLEVULKAN_EDITORSYSTEM_HPP