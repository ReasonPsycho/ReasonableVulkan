#pragma once

#include <string>
#include <string_view>
#include <rapidjson/document.h>
#include "ecs/Types.h"
#include "ecs/utility/Reflection.hpp"

namespace engine {
    class Engine;
}

namespace engine::ecs {

    class Scene;
    class ScriptSystem;
    struct ScriptComponent;
    class ScriptRegistry;
    class EditorSystem;

    class Script {
    public:
        virtual ~Script() = default;

        // Lifecycle hooks
        virtual void OnAwake() {}
        virtual void OnStart() {}
        virtual void OnUpdate(float /*deltaTime*/) {}
        virtual void OnDestroy() {}
        virtual void OnEnable() {}
        virtual void OnDisable() {}

        // Reflected polymorphic serialization
        virtual std::string_view GetTypeName() const { return typeName; }
        virtual void Serialize(rapidjson::Value& /*outObj*/, rapidjson::Document::AllocatorType& /*allocator*/) const {}
        virtual void Deserialize(const rapidjson::Value& /*inObj*/, Scene* /*scene*/) {}

        // Reflected ImGui UI
#ifdef ENABLE_IMGUI
        virtual bool DrawInspector(Scene* /*scene*/) { return false; }
#endif

        // ECS Helper Accessors
        template<typename T, typename... Args>
        T& AddComponent(Args&&... args);

        template<typename T>
        T& GetComponent();

        template<typename T>
        const T& GetComponent() const;

        template<typename T>
        bool HasComponent() const;

        template<typename T>
        void RemoveComponent();

        Entity GetEntity() const { return Entity(entity.GetID(), scene); }
        Scene* GetScene() const { return scene; }
        bool IsEnabled() const { return enabled; }

        void SetEnabled(bool state) {
            if (enabled != state) {
                enabled = state;
                if (started) {
                    if (enabled) {
                        OnEnable();
                    } else {
                        OnDisable();
                    }
                }
            }
        }

        bool HasStarted() const { return started; }

    protected:
        std::string typeName{"Script"};

    private:
        friend class ::engine::Engine;
        friend class ScriptSystem;
        friend struct ScriptComponent;
        friend class ScriptRegistry;
        friend class EditorSystem;

        Entity entity{MAX_ENTITIES};
        Scene* scene{nullptr};
        bool started{false};
        bool enabled{true};
    };

    template <typename Derived>
    class ScriptTemplate : public Script {
    public:
        ScriptTemplate() {
            constexpr auto name = std::meta::identifier_of(^^Derived);
            typeName = name;
        }

        std::string_view GetTypeName() const override {
            constexpr auto name = std::meta::identifier_of(^^Derived);
            return name;
        }
    };

    #define REFLECT_SCRIPT(ScriptType) \
        std::string_view GetTypeName() const override { \
            return #ScriptType; \
        } \
        void Serialize(rapidjson::Value& outObj, rapidjson::Document::AllocatorType& allocator) const override { \
            ::engine::ecs::SerializeTypeToJson(*this, outObj, allocator); \
        } \
        void Deserialize(const rapidjson::Value& inObj, ::engine::ecs::Scene* /*scene*/) override { \
            ::engine::ecs::DeserializeTypeFromJson(const_cast<ScriptType&>(*this), inObj); \
        } \
        bool DrawInspector(::engine::ecs::Scene* scene) override { \
            if constexpr (requires { const_cast<ScriptType&>(*this).CustomDrawImGui(scene); }) { \
                return const_cast<ScriptType&>(*this).CustomDrawImGui(scene); \
            } else { \
                return ::engine::ecs::DrawComponentFields(const_cast<ScriptType&>(*this), scene); \
            } \
        }

} // namespace engine::ecs

#include "Script.tpp"