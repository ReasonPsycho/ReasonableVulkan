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
#include <memory>
#include "systems/renderingSystem/componets/CameraComponent.hpp"
#include "systems/transformSystem/componets/TransformComponent.hpp"
#include "ecs/Types.h"

namespace plt
{
    class PlatformInterface;
}
namespace engine
{
    class Engine;
}
namespace engine::ecs
{
    struct Component;
    class Scene;

    class EditorSystem
    {
    public:
        explicit EditorSystem(::engine::Engine* engine);
        void Update(float deltaTime);

        struct ComponentInfo {
            std::string displayName;
            bool isIntegral = false;
            std::function<bool(Scene* scene, void* component)> showImGuiComponent;
        };

        // Register component type with a display name and ImGui renderer
        template<typename T>
        void RegisterComponentType() {
            ComponentInfo info;
            info.displayName = std::meta::identifier_of(^^T);
            info.isIntegral = has_annotation<Integral>(^^T);
            info.showImGuiComponent = [](Scene* scene, void* component) -> bool {
                if (!component) return false;
                bool shouldRemove = false;
                constexpr auto typeName = std::meta::identifier_of(^^T);
                bool open = ImGui::CollapsingHeader(typeName.data());
                if constexpr (!has_annotation<Integral>(^^T) && !std::is_same_v<T, TransformComponent>) {
                    if (ImGui::BeginPopupContextItem()) {
                        if (ImGui::MenuItem("Remove Component")) {
                            shouldRemove = true;
                        }
                        ImGui::EndPopup();
                    }
                }
                if (open) {
                    auto& comp = *static_cast<T*>(component);
                    if constexpr (requires { comp.CustomDrawImGui(scene); }) {
                        comp.CustomDrawImGui(scene);
                    } else {
                        DrawComponentFields(comp, scene);
                    }
                }
                return shouldRemove;
            };
            registeredComponentTypes[typeid(T)] = std::move(info);
        }

        void SetEntityName(Entity entity, const std::string& name, Scene* targetScene = nullptr);
        std::string GetEntityName(Entity entity, Scene* targetScene = nullptr) const;
        std::string GetEntityRawName(Entity entity, Scene* targetScene = nullptr) const;

        Entity GetSelectedEntity() const { return selectedEntity; }
        void SetSelectedEntity(Entity entity) { selectedEntity = entity; }

        Scene* GetTargetScene() const;
        void SetTargetScene(std::shared_ptr<Scene> scene) { selectedScene = scene; }

        [[=NonSerialized{}]]
        CameraComponent camera = CameraComponent();
        [[=NonSerialized{}]]
        TransformComponent cameraTransform = TransformComponent();

        [[=Tooltip{"Toggle in-editor editing mode"}]]
        bool inEditMode = true;

        void Initialize();

        enum class ShaderOverrideMode {
            Default,
            Wiremesh,
            TexturedWiremesh
        };

        [[=Tooltip{"Shader override mode for viewport rendering"}]]
        ShaderOverrideMode currentShaderOverride = ShaderOverrideMode::Default;
        [[=NonSerialized{}]]
        boost::uuids::uuid wiremeshShaderId = boost::uuids::nil_uuid();
        [[=NonSerialized{}]]
        boost::uuids::uuid wiremeshTexturedShaderId = boost::uuids::nil_uuid();
        [[=NonSerialized{}]]
        gfx::ShaderProgramHandle wiremeshShaderHandle = gfx::ShaderProgramHandle::invalid();
        [[=NonSerialized{}]]
        gfx::ShaderProgramHandle wiremeshTexturedShaderHandle = gfx::ShaderProgramHandle::invalid();

        void SetUpCameraControls();

        ::engine::Engine* engine = nullptr;

    private:
        std::weak_ptr<Scene> selectedScene;
        std::unordered_map<Entity, std::string> named_entities;
        Entity selectedEntity = std::numeric_limits<std::uint32_t>::max();
        Entity renamingEntity = std::numeric_limits<std::uint32_t>::max();
        char renameBuf[256] = "";
        bool renameFocusRequested = false;
        std::unordered_map<std::type_index,ComponentInfo> registeredComponentTypes;
        void ImGuiSceneGraph();
        void ImGuiGraphEntity(Scene* currentScene, Entity entity);
        void ImGuiInspector();
        void ImGuiSystemSettings();
        void ImGuiGizmo();
        void ImguiShaderOverrideWindow();
        void ImguiToolbar();

        bool isRightMousePressed = false;
        bool isLeftMousePressed = false;
        bool isMiddleMousePressed = false;
        float cameraDistance = 5.0f;
        float cameraYaw = 0.0f;
        float cameraPitch = 45.0f;
        glm::vec3 cameraTarget = glm::vec3(0.0f);

        void UpdateCameraPosition();
        ImVec2 lastViewportSize = { 0, 0 };
        ImVec2 lastViewportPos = { 0, 0 };
    };
}

#endif //REASONABLEVULKAN_EDITORSYSTEM_HPP