//
// Created by redkc on 09/10/2025.
//

#ifndef REASONABLEVULKAN_EDITORSYSTEM_HPP
#define REASONABLEVULKAN_EDITORSYSTEM_HPP
#include <functional>
#include <limits>
#include <typeindex>
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

        struct ComponentInfo {
            std::string displayName;
            std::function<void(void*)> imguiRenderer;
        };

        // Register component type with a display name and its ImGui renderer
        template<typename T>
        void RegisterComponentType(const std::string& displayName = "") {
            std::type_index typeIndex(typeid(T));
            ComponentInfo info;
            info.displayName = boost::core::demangle(typeid(T).name()) ;

            // Create a type-safe wrapper for ImGuiComponent
            info.imguiRenderer = [](void* component) {
                if (auto* typedComponent = static_cast<T*>(component)) {
                    typedComponent->ImGuiComponent();
                }
            };

            registeredComponents[typeIndex] = std::move(info);
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
        std::unordered_map<std::type_index, ComponentInfo> registeredComponents;
        void ImGuiSceneGraph();
        void ImGuiGraphEntity(Entity entity);
        void ImGuiInspector();

    };
}

#endif //REASONABLEVULKAN_EDITORSYSTEM_HPP