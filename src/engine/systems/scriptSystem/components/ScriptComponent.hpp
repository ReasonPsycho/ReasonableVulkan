#pragma once

#include <vector>
#include <memory>
#include <string>
#include <string_view>
#include "ecs/Component.hpp"
#include "ecs/utility/Reflection.hpp"
#include "../Script.hpp"
#include "../ScriptRegistry.hpp"

#ifdef ENABLE_IMGUI
#include <imgui.h>
#endif

namespace engine::ecs {

    struct ScriptComponent : public Component {
        [[=NonSerialized{}]]
        std::vector<std::shared_ptr<Script>> scripts;

        template<typename T, typename... Args>
        std::shared_ptr<T> AddScript(Args&&... args) {
            auto script = std::make_shared<T>(std::forward<Args>(args)...);
            constexpr auto name = std::meta::identifier_of(^^T);
            script->typeName = name;
            scripts.push_back(script);
            return script;
        }

        std::shared_ptr<Script> AddScriptByName(std::string_view name) {
            auto script = ScriptRegistry::Instance().Instantiate(name);
            if (script) {
                scripts.push_back(script);
            }
            return script;
        }

        template<typename T>
        std::shared_ptr<T> GetScript() const {
            for (const auto& script : scripts) {
                if (auto casted = std::dynamic_pointer_cast<T>(script)) {
                    return casted;
                }
            }
            return nullptr;
        }

        template<typename T>
        bool HasScript() const {
            return GetScript<T>() != nullptr;
        }

        template<typename T>
        bool RemoveScript() {
            for (auto it = scripts.begin(); it != scripts.end(); ++it) {
                if (std::dynamic_pointer_cast<T>(*it)) {
                    if (*it) {
                        (*it)->OnDestroy();
                    }
                    scripts.erase(it);
                    return true;
                }
            }
            return false;
        }

        void RemoveScriptByIndex(size_t index) {
            if (index < scripts.size()) {
                if (scripts[index]) {
                    scripts[index]->OnDestroy();
                }
                scripts.erase(scripts.begin() + index);
            }
        }

        void CustomSerialize(rapidjson::Value& outObj, rapidjson::Document::AllocatorType& allocator) const {
            outObj.SetObject();
            rapidjson::Value scriptArray(rapidjson::kArrayType);

            for (const auto& script : scripts) {
                if (!script) continue;
                rapidjson::Value scriptObj(rapidjson::kObjectType);

                auto typeName = script->GetTypeName();
                rapidjson::Value typeVal(typeName.data(), static_cast<rapidjson::SizeType>(typeName.size()), allocator);
                scriptObj.AddMember("type", typeVal, allocator);
                scriptObj.AddMember("enabled", script->IsEnabled(), allocator);

                rapidjson::Value fieldsObj(rapidjson::kObjectType);
                if (auto info = ScriptRegistry::Instance().GetTypeInfo(typeName)) {
                    info->serialize(script.get(), fieldsObj, allocator);
                } else {
                    script->Serialize(fieldsObj, allocator);
                }
                scriptObj.AddMember("fields", fieldsObj, allocator);

                scriptArray.PushBack(scriptObj, allocator);
            }

            outObj.AddMember("scripts", scriptArray, allocator);
        }

        void CaptureStateSnapshot(rapidjson::Value& outObj, rapidjson::Document::AllocatorType& allocator) const {
            CustomSerialize(outObj, allocator);
        }

        void CustomDeserialize(const rapidjson::Value& inObj, Scene* scene = nullptr) {
            scripts.clear();
            if (!inObj.IsObject() || !inObj.HasMember("scripts") || !inObj["scripts"].IsArray()) {
                return;
            }

            const auto& scriptArray = inObj["scripts"];
            for (rapidjson::SizeType i = 0; i < scriptArray.Size(); ++i) {
                const auto& item = scriptArray[i];
                if (!item.IsObject() || !item.HasMember("type") || !item["type"].IsString()) {
                    continue;
                }

                std::string typeName = item["type"].GetString();
                auto script = ScriptRegistry::Instance().Instantiate(typeName);
                if (script) {
                    if (item.HasMember("enabled") && item["enabled"].IsBool()) {
                        script->SetEnabled(item["enabled"].GetBool());
                    }
                    if (item.HasMember("fields") && item["fields"].IsObject()) {
                        if (auto info = ScriptRegistry::Instance().GetTypeInfo(typeName)) {
                            info->deserialize(script.get(), item["fields"], scene);
                        } else {
                            script->Deserialize(item["fields"], scene);
                        }
                    }
                    scripts.push_back(script);
                }
            }
        }

        void RestoreStateSnapshot(const rapidjson::Value& inObj, Scene* scene = nullptr) {
            CustomDeserialize(inObj, scene);
        }

#ifdef ENABLE_IMGUI
        void CustomDrawImGui(Scene* scene) {
            int toRemove = -1;
            for (size_t i = 0; i < scripts.size(); ++i) {
                auto& script = scripts[i];
                if (!script) continue;

                ImGui::PushID(static_cast<int>(i));
                bool enabled = script->IsEnabled();
                if (ImGui::Checkbox("##enabled", &enabled)) {
                    script->SetEnabled(enabled);
                }
                ImGui::SameLine();

                std::string headerName = std::string(script->GetTypeName());
                if (ImGui::TreeNode(headerName.c_str())) {
                    if (auto info = ScriptRegistry::Instance().GetTypeInfo(script->GetTypeName())) {
                        info->drawInspector(script.get(), scene);
                    } else {
                        script->DrawInspector(scene);
                    }

                    if (ImGui::Button("Remove Script")) {
                        toRemove = static_cast<int>(i);
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }

            if (toRemove >= 0 && toRemove < static_cast<int>(scripts.size())) {
                RemoveScriptByIndex(static_cast<size_t>(toRemove));
            }

            // Dropdown to add registered scripts
            auto registeredNames = ScriptRegistry::Instance().GetRegisteredNames();
            if (!registeredNames.empty()) {
                if (ImGui::BeginCombo("Add Script", "Select Script...")) {
                    for (const auto& name : registeredNames) {
                        if (ImGui::Selectable(name.c_str())) {
                            auto script = AddScriptByName(name);
                            if (script && scene) {
                                script->scene = scene;
                            }
                        }
                    }
                    ImGui::EndCombo();
                }
            }
        }
#endif
    };

} // namespace engine::ecs
