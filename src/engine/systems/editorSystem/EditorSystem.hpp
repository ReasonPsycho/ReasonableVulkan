//
// Created by redkc on 09/10/2025.
//

#ifndef REASONABLEVULKAN_EDITORSYSTEM_HPP
#define REASONABLEVULKAN_EDITORSYSTEM_HPP
#include <functional>
#include <limits>
#include <typeindex>
#include <unordered_map>
#include <glm/glm.hpp>
#include <utility>
#include "ecs/System.h"

namespace engine::ecs
{
    struct Component;

    class EditorSystem :  public System<EditorSystem>
    {
    public:
        EditorSystem(Scene* scene) : System(scene) {}
        void Update(float deltaTime) override;

        struct ComponentInfo {
            std::string displayName;
            std::function<void(Scene* scene, Component* component)> showImGuiComponent;
         };

        // Register component type with a display name and ImGui renderer
        template<typename T>
        void RegisterComponentType() {
            ComponentInfo info;
            info.displayName = boost::core::demangle(typeid(T).name());
            info.showImGuiComponent = [](Scene* scene, Component* component) {
                if (auto* typedComponent = dynamic_cast<T*>(component)) {
                    typedComponent->ShowImGui(scene, component);
                }
            };
            registeredComponentTypes[typeid(T)] = std::move(info);
        }


        void SetEntityName(Entity entity, const std::string& name);
        std::string GetEntityName(Entity entity) const;

        Entity GetSelectedEntity() const { return selectedEntity; }
        void SetSelectedEntity(Entity entity) { selectedEntity = entity; }

    protected:
        void OnEntityAdded(Entity entity) override {}
        void OnEntityRemoved(Entity entity) override {}

    private:
        std::unordered_map<Entity, std::string> named_entities;
        Entity selectedEntity = std::numeric_limits<std::uint32_t>::max();;
        std::unordered_map<std::type_index,ComponentInfo> registeredComponentTypes;
        void ImGuiSceneGraph();
        void ImGuiGraphEntity(Entity entity);
        void ImGuiInspector();
        void ImGuiGizmo();
        void ImguiToolbar();
        void ImguiMenu();
    };
}

#endif //REASONABLEVULKAN_EDITORSYSTEM_HPP