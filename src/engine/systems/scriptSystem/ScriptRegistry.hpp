#pragma once

#include <string>
#include <string_view>
#include <functional>
#include <unordered_map>
#include <memory>
#include <vector>
#include <meta>
#include "Script.hpp"
#include "ecs/utility/Reflection.hpp"

#if defined(_WIN32)
    #if defined(GAMEPLAY_SCRIPTS_EXPORTS)
        #define SCRIPT_API __declspec(dllexport)
    #else
        #define SCRIPT_API __declspec(dllimport)
    #endif
#else
    #define SCRIPT_API __attribute__((visibility("default")))
#endif

namespace engine::ecs {

    using ScriptCreateFn = std::shared_ptr<Script>(*)();
    using ScriptSerializeFn = void(*)(const Script*, rapidjson::Value&, rapidjson::Document::AllocatorType&);
    using ScriptDeserializeFn = void(*)(Script*, const rapidjson::Value&, Scene*);
#ifdef ENABLE_IMGUI
    using ScriptDrawInspectorFn = bool(*)(Script*, Scene*);
#endif

    struct ScriptExportEntry {
        const char* name{nullptr};
        ScriptCreateFn create{nullptr};
        ScriptSerializeFn serialize{nullptr};
        ScriptDeserializeFn deserialize{nullptr};
#ifdef ENABLE_IMGUI
        ScriptDrawInspectorFn drawInspector{nullptr};
#endif
    };

    struct ScriptTypeInfo {
        std::string name;
        ScriptCreateFn create{nullptr};
        ScriptSerializeFn serialize{nullptr};
        ScriptDeserializeFn deserialize{nullptr};
#ifdef ENABLE_IMGUI
        ScriptDrawInspectorFn drawInspector{nullptr};
#endif
    };

    class ScriptRegistry {
    public:
        static ScriptRegistry& Instance() {
            static ScriptRegistry instance;
            return instance;
        }

        void Clear() {
            typeInfos.clear();
        }

        void RegisterScriptRaw(std::string_view name,
                               ScriptCreateFn create,
                               ScriptSerializeFn serialize,
                               ScriptDeserializeFn deserialize
#ifdef ENABLE_IMGUI
                               , ScriptDrawInspectorFn drawInspector = nullptr
#endif
        );

        template <typename T>
        static std::shared_ptr<Script> CreateScriptInstance() {
            constexpr auto name = std::meta::identifier_of(^^T);
            auto script = std::make_shared<T>();
            script->typeName = std::string(name);
            return script;
        }

        template <typename T>
        static void SerializeScriptInstance(const Script* s, rapidjson::Value& outObj, rapidjson::Document::AllocatorType& alloc) {
            SerializeTypeToJson(*static_cast<const T*>(s), outObj, alloc);
        }

        template <typename T>
        static void DeserializeScriptInstance(Script* s, const rapidjson::Value& inObj, Scene* /*scene*/) {
            DeserializeTypeFromJson(*static_cast<T*>(s), inObj);
        }

#ifdef ENABLE_IMGUI
        template <typename T>
        static bool DrawScriptInspector(Script* s, Scene* scene) {
            auto* typed = static_cast<T*>(s);
            if constexpr (requires { typed->CustomDrawImGui(scene); }) {
                return typed->CustomDrawImGui(scene);
            } else {
                return DrawComponentFields(*typed, scene);
            }
        }
#endif

        template <typename T>
        void RegisterScript() {
            constexpr auto name = std::meta::identifier_of(^^T);
#ifdef ENABLE_IMGUI
            RegisterScriptRaw(name,
                              &CreateScriptInstance<T>,
                              &SerializeScriptInstance<T>,
                              &DeserializeScriptInstance<T>,
                              &DrawScriptInspector<T>);
#else
            RegisterScriptRaw(name,
                              &CreateScriptInstance<T>,
                              &SerializeScriptInstance<T>,
                              &DeserializeScriptInstance<T>);
#endif
        }

        std::shared_ptr<Script> Instantiate(std::string_view name) const {
            auto it = typeInfos.find(std::string(name));
            if (it != typeInfos.end() && it->second.create) {
                return it->second.create();
            }
            return nullptr;
        }

        const ScriptTypeInfo* GetTypeInfo(std::string_view name) const {
            auto it = typeInfos.find(std::string(name));
            if (it != typeInfos.end()) {
                return &it->second;
            }
            return nullptr;
        }

        std::vector<std::string> GetRegisteredNames() const {
            std::vector<std::string> names;
            names.reserve(typeInfos.size());
            for (const auto& [name, _] : typeInfos) {
                names.push_back(name);
            }
            return names;
        }

        bool HasScript(std::string_view name) const {
            return typeInfos.find(std::string(name)) != typeInfos.end();
        }

    private:
        ScriptRegistry() = default;
        std::unordered_map<std::string, ScriptTypeInfo> typeInfos;
    };

    template <typename T>
    struct ScriptAutoRegister {
        explicit ScriptAutoRegister(std::string_view /*name*/ = "") {}
    };

    #define REGISTER_SCRIPT(ScriptType)

    template <typename... ScriptTypes>
    struct ScriptTupleRegistry {
        using RegistryTuple = std::tuple<ScriptTypes...>;

        static void RegisterAll() {
            ForEachType<RegistryTuple>([]<typename T>() {
                ScriptRegistry::Instance().RegisterScript<T>();
            });
        }
    };

    void InitializeScriptRegistry();

} // namespace engine::ecs
