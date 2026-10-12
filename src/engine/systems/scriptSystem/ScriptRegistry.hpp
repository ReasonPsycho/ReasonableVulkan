#pragma once

#include <string>
#include <string_view>
#include <functional>
#include <unordered_map>
#include <memory>
#include <vector>
#include <meta>
#include "Script.hpp"
#include "ecs/Reflection.hpp"

namespace engine::ecs {

    struct ScriptTypeInfo {
        std::string name;
        std::function<std::shared_ptr<Script>()> create;
        std::function<void(const Script*, rapidjson::Value&, rapidjson::Document::AllocatorType&)> serialize;
        std::function<void(Script*, const rapidjson::Value&, Scene*)> deserialize;
#ifdef ENABLE_IMGUI
        std::function<bool(Script*, Scene*)> drawInspector;
#endif
    };

    class ScriptRegistry {
    public:
        using ScriptFactory = std::function<std::shared_ptr<Script>()>;

        static ScriptRegistry& Instance() {
            static ScriptRegistry instance;
            return instance;
        }

        void Register(std::string_view name, ScriptFactory factory) {
            std::string nameStr(name);
            factories[nameStr] = std::move(factory);
        }

        template <typename T>
        void RegisterScript() {
            constexpr auto name = std::meta::identifier_of(^^T);
            std::string nameStr(name);

            ScriptTypeInfo info;
            info.name = nameStr;
            info.create = [nameStr]() -> std::shared_ptr<Script> {
                auto script = std::make_shared<T>();
                return script;
            };
            info.serialize = [](const Script* s, rapidjson::Value& outObj, rapidjson::Document::AllocatorType& alloc) {
                SerializeTypeToJson(*static_cast<const T*>(s), outObj, alloc);
            };
            info.deserialize = [](Script* s, const rapidjson::Value& inObj, Scene* /*scene*/) {
                DeserializeTypeFromJson(*static_cast<T*>(s), inObj);
            };
#ifdef ENABLE_IMGUI
            info.drawInspector = [](Script* s, Scene* scene) -> bool {
                auto* typed = static_cast<T*>(s);
                if constexpr (requires { typed->CustomDrawImGui(scene); }) {
                    return typed->CustomDrawImGui(scene);
                } else {
                    return DrawComponentFields(*typed, scene);
                }
            };
#endif
            typeInfos[nameStr] = std::move(info);
            factories[nameStr] = [this, nameStr]() {
                auto it = typeInfos.find(nameStr);
                return it != typeInfos.end() ? it->second.create() : nullptr;
            };
        }

        std::shared_ptr<Script> Instantiate(std::string_view name) const {
            auto it = factories.find(std::string(name));
            if (it != factories.end()) {
                return it->second();
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
            names.reserve(factories.size());
            for (const auto& [name, _] : factories) {
                names.push_back(name);
            }
            return names;
        }

        bool HasScript(std::string_view name) const {
            return factories.find(std::string(name)) != factories.end();
        }

    private:
        ScriptRegistry() = default;
        std::unordered_map<std::string, ScriptFactory> factories;
        std::unordered_map<std::string, ScriptTypeInfo> typeInfos;
    };

    template <typename T>
    struct ScriptAutoRegister {
        explicit ScriptAutoRegister(std::string_view name = "") {
            ScriptRegistry::Instance().RegisterScript<T>();
        }
    };

    #define REGISTER_SCRIPT(ScriptType) \
        inline static const ::engine::ecs::ScriptAutoRegister<ScriptType> _auto_reg_##ScriptType{#ScriptType};

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
