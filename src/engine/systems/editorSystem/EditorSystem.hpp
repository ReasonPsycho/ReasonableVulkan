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
            std::function<void(Scene* scene,Component* componentData)> showImGuiComponent;
        };

        // Register component type with a display name and its ImGui renderer
        template<typename T>
        void RegisterComponentType(std::function<void(Scene* scene, T* componentData)> showImGuiComponent) {
            ComponentInfo newComponentInfo;
            newComponentInfo.displayName = boost::core::demangle(typeid(T).name());
            // Create a wrapper that performs the type cast
            newComponentInfo.showImGuiComponent = [showImGuiComponent](Scene* scene, Component* componentData) {
                T* derived = static_cast<T*>(componentData);
                showImGuiComponent(scene, derived);
            };
            registeredShowImGuiComponents[typeid(T)] = newComponentInfo;
        }

        void RegisterShowImGuiComponent(std::type_index typeIndex,std::function<void(Scene* scene)> function) {
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
        std::unordered_map<std::type_index,ComponentInfo> registeredShowImGuiComponents;
        void ImGuiSceneGraph();
        void ImGuiGraphEntity(Entity entity);
        void ImGuiInspector();
        void ImGuiGizmo();
        void ImguiToolbar();
        void ImguiMenu();
    };
}

#endif //REASONABLEVULKAN_EDITORSYSTEM_HPP