//
// Created by Junie on 2026.
// Static reflection and attribute system for ReasonableVulkan ECS.
//

#ifndef REASONABLEVULKAN_REFLECTION_HPP
#define REASONABLEVULKAN_REFLECTION_HPP

#include <meta>
#include <array>
#include <optional>
#include <string_view>
#include <string>
#include <variant>
#include <type_traits>
#include <concepts>
#include <set>
#include <typeindex>
#include <stdexcept>
#include <tuple>
#include <cstdio>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <rapidjson/document.h>

#ifdef ENABLE_IMGUI
#include <imgui.h>
#endif

namespace engine::ecs
{
    class Scene;

    // ==========================================
    // Attributes
    // ==========================================

    struct Range {
        float min = 0.0f;
        float max = 0.0f;
        float speed = 0.1f;
    };

    struct Tooltip {
        char text[128]{};

        constexpr Tooltip() = default;

        constexpr explicit Tooltip(const char* str) {
            if (str) {
                int i = 0;
                while (str[i] && i < 127) {
                    text[i] = str[i];
                    ++i;
                }
                text[i] = '\0';
            }
        }
    };

    struct Color {};
    struct ReadOnly {};
    struct NonSerialized {};
    struct Integral {};

    // ==========================================
    // Compile-time Reflection Helpers
    // ==========================================

    template <std::size_t N>
    struct MetaInfoArray {
        std::meta::info data[N > 0 ? N : 1]{};
        std::size_t size_val = N;

        consteval std::size_t size() const noexcept { return size_val; }
        consteval std::meta::info operator[](std::size_t i) const noexcept { return data[i]; }
        consteval const std::meta::info* begin() const noexcept { return data; }
        consteval const std::meta::info* end() const noexcept { return data + N; }
    };

    template <typename Tuple>
    consteval auto get_template_args_array() {
        auto vec = std::meta::template_arguments_of(^^Tuple);
        constexpr std::size_t N = std::tuple_size_v<Tuple>;
        MetaInfoArray<N> arr{};
        for (std::size_t i = 0; i < vec.size(); ++i) {
            arr.data[i] = vec[i];
        }
        return arr;
    }

    template <typename TypeTuple, typename Func>
    constexpr void ForEachType(Func&& func) {
        static constexpr auto types = get_template_args_array<TypeTuple>();
        template for (constexpr auto type : types) {
            func.template operator()<typename [:type:]>();
        }
    }

    template <typename T, typename Tuple>
    consteval std::size_t IndexInTuple() {
        constexpr auto types = get_template_args_array<Tuple>();
        for (std::size_t i = 0; i < types.size(); ++i) {
            if (types[i] == ^^T) return i;
        }
        return std::numeric_limits<std::size_t>::max();
    }

    template <typename Tuple>
    constexpr std::string_view GetTypeNameByIndex(std::size_t index) {
        static constexpr auto types = get_template_args_array<Tuple>();
        if (index < types.size()) {
            template for (constexpr auto type : types) {
                constexpr auto idx = IndexInTuple<typename [:type:], Tuple>();
                if (index == idx) {
                    return std::meta::identifier_of(type);
                }
            }
        }
        return "";
    }

    template <typename Tuple>
    inline std::optional<std::type_index> GetTypeByName(std::string_view name) {
        static constexpr auto types = get_template_args_array<Tuple>();
        template for (constexpr auto type : types) {
            if (name == std::meta::identifier_of(type)) {
                return std::type_index(typeid(typename [:type:]));
            }
        }
        return std::nullopt;
    }

    template <typename Tuple>
    inline std::string_view GetTypeName(const std::type_index& typeIdx) {
        static constexpr auto types = get_template_args_array<Tuple>();
        template for (constexpr auto type : types) {
            if (typeIdx == std::type_index(typeid(typename [:type:]))) {
                return std::meta::identifier_of(type);
            }
        }
        return typeIdx.name();
    }

    template <typename Tuple>
    inline std::size_t GetTypeIndex(const std::type_index& typeIdx) {
        static constexpr auto types = get_template_args_array<Tuple>();
        template for (constexpr auto type : types) {
            if (typeIdx == std::type_index(typeid(typename [:type:]))) {
                return IndexInTuple<typename [:type:], Tuple>();
            }
        }
        throw std::runtime_error("Type not found in type list for: " + std::string(typeIdx.name()));
    }

    template <typename Tuple>
    inline std::type_index GetTypeByIndex(std::size_t index) {
        static constexpr auto types = get_template_args_array<Tuple>();
        template for (constexpr auto type : types) {
            constexpr auto idx = IndexInTuple<typename [:type:], Tuple>();
            if (index == idx) {
                return std::type_index(typeid(typename [:type:]));
            }
        }
        throw std::out_of_range("Type index out of range: " + std::to_string(index));
    }

    template <typename Tuple>
    inline const std::set<std::type_index>& GetRegisteredTypesSet() {
        static const std::set<std::type_index> types = []() {
            std::set<std::type_index> set;
            ForEachType<Tuple>([&set]<typename T>() {
                set.insert(std::type_index(typeid(T)));
            });
            return set;
        }();
        return types;
    }

    template <typename Attr>
    consteval bool has_annotation(std::meta::info entity) {
        auto annots = std::meta::annotations_of_with_type(entity, ^^Attr);
        return !annots.empty();
    }

    template <std::meta::info entity>
    consteval bool is_nonSerialized() {
        return has_annotation<NonSerialized>(entity);
    }

    template <typename Attr>
    consteval std::optional<Attr> get_annotation(std::meta::info entity) {
        auto annots = std::meta::annotations_of_with_type(entity, ^^Attr);
        if (!annots.empty()) {
            return std::meta::extract<Attr>(annots[0]);
        }
        return std::nullopt;
    }

    template <typename T>
    consteval auto member_count() {
        return std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()).size();
    }

    template <typename T>
    consteval auto get_members_array() {
        auto vec = std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current());
        constexpr std::size_t N = member_count<T>();
        MetaInfoArray<N> arr{};
        for (std::size_t i = 0; i < vec.size(); ++i) {
            arr.data[i] = vec[i];
        }
        return arr;
    }

    // ==========================================
    // Generic JSON Serialization
    // ==========================================

    template <typename T>
    void SerializeValueToJson(const T& val, rapidjson::Value& outVal, rapidjson::Document::AllocatorType& allocator);

    template <typename T>
    void SerializeTypeToJson(const T& component, rapidjson::Value& outObj, rapidjson::Document::AllocatorType& allocator) {
        if (!outObj.IsObject()) {
            outObj.SetObject();
        }

        static constexpr auto members = get_members_array<T>();
        template for (constexpr auto mem : members) {
            if constexpr (!is_nonSerialized<mem>()) {
                constexpr auto name = std::meta::identifier_of(mem);
                const auto& memberVal = component.[:mem:];
                rapidjson::Value key(name.data(), static_cast<rapidjson::SizeType>(name.size()), allocator);
                rapidjson::Value fieldVal;
                SerializeValueToJson(memberVal, fieldVal, allocator);
                outObj.AddMember(key, fieldVal, allocator);
            }
        }
    }

    template <typename... Ts>
    struct is_variant : std::false_type {};
    template <typename... Ts>
    struct is_variant<std::variant<Ts...>> : std::true_type {};
    template <typename T>
    inline constexpr bool is_variant_v = is_variant<T>::value;

    template <typename T>
    void SerializeValueToJson(const T& val, rapidjson::Value& outVal, rapidjson::Document::AllocatorType& allocator) {
        using RawT = std::decay_t<T>;
        if constexpr (std::is_same_v<RawT, float>) {
            outVal.SetFloat(val);
        } else if constexpr (std::is_same_v<RawT, double>) {
            outVal.SetDouble(val);
        } else if constexpr (std::is_same_v<RawT, int>) {
            outVal.SetInt(val);
        } else if constexpr (std::is_same_v<RawT, unsigned int>) {
            outVal.SetUint(val);
        } else if constexpr (std::is_same_v<RawT, uint64_t> || std::is_same_v<RawT, size_t>) {
            outVal.SetUint64(val);
        } else if constexpr (std::is_same_v<RawT, bool>) {
            outVal.SetBool(val);
        } else if constexpr (std::is_enum_v<RawT>) {
            outVal.SetInt(static_cast<int>(val));
        } else if constexpr (std::is_same_v<RawT, std::string>) {
            outVal.SetString(val.c_str(), static_cast<rapidjson::SizeType>(val.size()), allocator);
        } else if constexpr (std::is_same_v<RawT, boost::uuids::uuid>) {
            std::string str = boost::uuids::to_string(val);
            outVal.SetString(str.c_str(), static_cast<rapidjson::SizeType>(str.size()), allocator);
        } else if constexpr (std::is_same_v<RawT, glm::vec2>) {
            outVal.SetArray();
            outVal.PushBack(val.x, allocator).PushBack(val.y, allocator);
        } else if constexpr (std::is_same_v<RawT, glm::vec3>) {
            outVal.SetArray();
            outVal.PushBack(val.x, allocator).PushBack(val.y, allocator).PushBack(val.z, allocator);
        } else if constexpr (std::is_same_v<RawT, glm::vec4>) {
            outVal.SetArray();
            outVal.PushBack(val.x, allocator).PushBack(val.y, allocator).PushBack(val.z, allocator).PushBack(val.w, allocator);
        } else if constexpr (std::is_same_v<RawT, glm::quat>) {
            outVal.SetArray();
            outVal.PushBack(val.w, allocator).PushBack(val.x, allocator).PushBack(val.y, allocator).PushBack(val.z, allocator);
        } else if constexpr (is_variant_v<RawT>) {
            std::visit([&](const auto& inner) {
                SerializeValueToJson(inner, outVal, allocator);
            }, val);
        } else if constexpr (std::is_class_v<RawT>) {
            SerializeTypeToJson(val, outVal, allocator);
        }
    }

    // ==========================================
    // Generic JSON Deserialization
    // ==========================================

    template <typename T>
    void DeserializeValueFromJson(T& val, const rapidjson::Value& inVal);

    template <typename T>
    void DeserializeTypeFromJson(T& component, const rapidjson::Value& inObj) {
        if (!inObj.IsObject()) return;

        static constexpr auto members = get_members_array<T>();
        template for (constexpr auto mem : members) {
            if constexpr (!is_nonSerialized<mem>()) {
                constexpr auto name = std::meta::identifier_of(mem);
                if (inObj.HasMember(name.data())) {
                    DeserializeValueFromJson(component.[:mem:], inObj[name.data()]);
                }
            }
        }

        if constexpr (requires { component.PostDeserialize(); }) {
            component.PostDeserialize();
        } else if constexpr (requires { component.isDirty = true; }) {
            component.isDirty = true;
        }
    }

    template <typename T>
    void DeserializeValueFromJson(T& val, const rapidjson::Value& inVal) {
        using RawT = std::decay_t<T>;
        if constexpr (std::is_same_v<RawT, float>) {
            if (inVal.IsNumber()) val = inVal.GetFloat();
        } else if constexpr (std::is_same_v<RawT, double>) {
            if (inVal.IsNumber()) val = inVal.GetDouble();
        } else if constexpr (std::is_same_v<RawT, int>) {
            if (inVal.IsInt()) val = inVal.GetInt();
        } else if constexpr (std::is_same_v<RawT, unsigned int>) {
            if (inVal.IsUint()) val = inVal.GetUint();
        } else if constexpr (std::is_same_v<RawT, uint64_t> || std::is_same_v<RawT, size_t>) {
            if (inVal.IsUint64()) val = inVal.GetUint64();
        } else if constexpr (std::is_same_v<RawT, bool>) {
            if (inVal.IsBool()) val = inVal.GetBool();
        } else if constexpr (std::is_enum_v<RawT>) {
            if (inVal.IsInt()) val = static_cast<RawT>(inVal.GetInt());
        } else if constexpr (std::is_same_v<RawT, std::string>) {
            if (inVal.IsString()) val = inVal.GetString();
        } else if constexpr (std::is_same_v<RawT, boost::uuids::uuid>) {
            if (inVal.IsString()) {
                try {
                    val = boost::uuids::string_generator()(inVal.GetString());
                } catch (...) {
                    val = boost::uuids::nil_uuid();
                }
            }
        } else if constexpr (std::is_same_v<RawT, glm::vec2>) {
            if (inVal.IsArray() && inVal.Size() >= 2) {
                val.x = inVal[0].GetFloat();
                val.y = inVal[1].GetFloat();
            }
        } else if constexpr (std::is_same_v<RawT, glm::vec3>) {
            if (inVal.IsArray() && inVal.Size() >= 3) {
                val.x = inVal[0].GetFloat();
                val.y = inVal[1].GetFloat();
                val.z = inVal[2].GetFloat();
            }
        } else if constexpr (std::is_same_v<RawT, glm::vec4>) {
            if (inVal.IsArray() && inVal.Size() >= 4) {
                val.x = inVal[0].GetFloat();
                val.y = inVal[1].GetFloat();
                val.z = inVal[2].GetFloat();
                val.w = inVal[3].GetFloat();
            }
        } else if constexpr (std::is_same_v<RawT, glm::quat>) {
            if (inVal.IsArray() && inVal.Size() >= 4) {
                val.w = inVal[0].GetFloat();
                val.x = inVal[1].GetFloat();
                val.y = inVal[2].GetFloat();
                val.z = inVal[3].GetFloat();
            }
        } else if constexpr (is_variant_v<RawT>) {
            std::visit([&](auto& inner) {
                DeserializeValueFromJson(inner, inVal);
            }, val);
        } else if constexpr (std::is_class_v<RawT>) {
            DeserializeTypeFromJson(val, inVal);
        }
    }

    // ==========================================
    // Generic ImGui Inspection
    // ==========================================

#ifdef ENABLE_IMGUI

    template <std::meta::info Mem, typename T>
    bool DrawFieldInspector(const char* name, T& value, Scene* scene);

    template <typename T>
    bool DrawComponentFields(T& component, Scene* scene) {
        bool changed = false;
        static constexpr auto members = get_members_array<T>();
        template for (constexpr auto mem : members) {
            constexpr auto name = std::meta::identifier_of(mem);
            constexpr auto tooltipOpt = get_annotation<Tooltip>(mem);

            if (DrawFieldInspector<mem>(name.data(), component.[:mem:], scene)) {
                changed = true;
            }

            if constexpr (tooltipOpt.has_value()) {
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                    ImGui::SetTooltip("%s", tooltipOpt->text);
                }
            }
        }

        if (changed) {
            if constexpr (requires { component.isDirty = true; }) {
                component.isDirty = true;
            }
        }

        return changed;
    }

    template <std::meta::info Mem, typename T>
    bool DrawFieldInspector(const char* name, T& value, Scene* scene) {
        using RawT = std::decay_t<T>;
        constexpr bool isReadOnly = has_annotation<ReadOnly>(Mem);
        constexpr auto rangeOpt = get_annotation<Range>(Mem);
        float speed = rangeOpt ? rangeOpt->speed : 0.1f;
        float minVal = rangeOpt ? rangeOpt->min : 0.0f;
        float maxVal = rangeOpt ? rangeOpt->max : 0.0f;
        bool changed = false;

        if constexpr (std::is_same_v<RawT, float>) {
            if constexpr (isReadOnly) ImGui::BeginDisabled();
            if (minVal != maxVal) {
                changed = ImGui::SliderFloat(name, &value, minVal, maxVal);
            } else {
                changed = ImGui::DragFloat(name, &value, speed, minVal, maxVal);
            }
            if constexpr (isReadOnly) ImGui::EndDisabled();
        } else if constexpr (std::is_same_v<RawT, double>) {
            if constexpr (isReadOnly) ImGui::BeginDisabled();
            float f = static_cast<float>(value);
            if (minVal != maxVal) {
                changed = ImGui::SliderFloat(name, &f, minVal, maxVal);
            } else {
                changed = ImGui::DragFloat(name, &f, speed, minVal, maxVal);
            }
            if (changed) value = static_cast<double>(f);
            if constexpr (isReadOnly) ImGui::EndDisabled();
        } else if constexpr (std::is_same_v<RawT, int>) {
            if constexpr (isReadOnly) ImGui::BeginDisabled();
            changed = ImGui::DragInt(name, &value, speed, static_cast<int>(minVal), static_cast<int>(maxVal));
            if constexpr (isReadOnly) ImGui::EndDisabled();
        } else if constexpr (std::is_same_v<RawT, bool>) {
            if constexpr (isReadOnly) ImGui::BeginDisabled();
            changed = ImGui::Checkbox(name, &value);
            if constexpr (isReadOnly) ImGui::EndDisabled();
        } else if constexpr (std::is_same_v<RawT, glm::vec2>) {
            if constexpr (isReadOnly) ImGui::BeginDisabled();
            changed = ImGui::DragFloat2(name, &value.x, speed, minVal, maxVal);
            if constexpr (isReadOnly) ImGui::EndDisabled();
        } else if constexpr (std::is_same_v<RawT, glm::vec3>) {
            if constexpr (isReadOnly) ImGui::BeginDisabled();
            if constexpr (has_annotation<Color>(Mem)) {
                changed = ImGui::ColorEdit3(name, &value.x);
            } else {
                changed = ImGui::DragFloat3(name, &value.x, speed, minVal, maxVal);
            }
            if constexpr (isReadOnly) ImGui::EndDisabled();
        } else if constexpr (std::is_same_v<RawT, glm::vec4>) {
            if constexpr (isReadOnly) ImGui::BeginDisabled();
            if constexpr (has_annotation<Color>(Mem)) {
                changed = ImGui::ColorEdit4(name, &value.x);
            } else {
                changed = ImGui::DragFloat4(name, &value.x, speed, minVal, maxVal);
            }
            if constexpr (isReadOnly) ImGui::EndDisabled();
        } else if constexpr (std::is_same_v<RawT, glm::quat>) {
            if constexpr (isReadOnly) ImGui::BeginDisabled();
            glm::vec3 euler = glm::degrees(glm::eulerAngles(value));
            if (ImGui::DragFloat3(name, &euler.x, speed)) {
                value = glm::quat(glm::radians(euler));
                changed = true;
            }
            if constexpr (isReadOnly) ImGui::EndDisabled();
        } else if constexpr (std::is_same_v<RawT, glm::mat4>) {
            if (ImGui::TreeNode(name)) {
                for (int i = 0; i < 4; ++i) {
                    ImGui::Text("%.2f %.2f %.2f %.2f", value[i].x, value[i].y, value[i].z, value[i].w);
                }
                ImGui::TreePop();
            }
        } else if constexpr (std::is_same_v<RawT, boost::uuids::uuid>) {
            std::string idStr = boost::uuids::to_string(value);
            ImGui::LabelText(name, "%s", idStr.c_str());
        } else if constexpr (std::is_same_v<RawT, std::string>) {
            if constexpr (isReadOnly) ImGui::BeginDisabled();
            char buffer[256];
            std::snprintf(buffer, sizeof(buffer), "%s", value.c_str());
            if (ImGui::InputText(name, buffer, sizeof(buffer))) {
                value = buffer;
                changed = true;
            }
            if constexpr (isReadOnly) ImGui::EndDisabled();
        } else if constexpr (is_variant_v<RawT>) {
            std::visit([&](auto& inner) {
                if (DrawComponentFields(inner, scene)) {
                    changed = true;
                }
            }, value);
        } else if constexpr (std::is_enum_v<RawT>) {
            if constexpr (isReadOnly) ImGui::BeginDisabled();
            int currentVal = static_cast<int>(value);
            if (ImGui::DragInt(name, &currentVal, 1.0f, static_cast<int>(minVal), static_cast<int>(maxVal))) {
                value = static_cast<RawT>(currentVal);
                changed = true;
            }
            if constexpr (isReadOnly) ImGui::EndDisabled();
        } else if constexpr (std::is_class_v<RawT>) {
            if (ImGui::TreeNode(name)) {
                if (DrawComponentFields(value, scene)) {
                    changed = true;
                }
                ImGui::TreePop();
            }
        }

        return changed;
    }

    template <typename T>
    void DrawComponentInspector(T& component, Scene* scene = nullptr) {
        constexpr auto typeName = std::meta::identifier_of(^^T);
        if (ImGui::CollapsingHeader(typeName.data())) {
            if constexpr (requires { component.CustomDrawImGui(scene); }) {
                component.CustomDrawImGui(scene);
            } else {
                DrawComponentFields(component, scene);
            }
        }
    }

#endif // ENABLE_IMGUI

}

#endif // REASONABLEVULKAN_REFLECTION_HPP
