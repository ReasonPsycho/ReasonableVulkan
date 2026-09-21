//
// Created by Junie on 2026.
// Static reflection and attribute system for ReasonableVulkan Asset Manager.
//

#ifndef REASONABLEVULKAN_ASSETREFLECTION_HPP
#define REASONABLEVULKAN_ASSETREFLECTION_HPP

#include <meta>
#include <string_view>
#include <string>
#include <optional>
#include <type_traits>
#include <concepts>
#include <tuple>
#include <vector>
#include <memory>
#include <limits>
#include <typeindex>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/functional/hash.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <rapidjson/document.h>

namespace am {
    class AssetInfo;
    class AssetManagerInterface;

    // Helper functions declared here, defined after AssetInfo / AssetManagerInterface definition
    boost::uuids::uuid GetAssetInfoId(const std::shared_ptr<AssetInfo>& info);
    size_t GetAssetInfoContentHash(const std::shared_ptr<AssetInfo>& info);
    std::shared_ptr<AssetInfo> ResolveAssetInfo(const boost::uuids::uuid& id, AssetManagerInterface* assetManager);

    // ==========================================
    // Attributes
    // ==========================================
    struct JsonName {
        char name[64]{};

        constexpr JsonName() = default;
        constexpr explicit JsonName(const char* str) {
            if (str) {
                int i = 0;
                while (str[i] && i < 63) {
                    name[i] = str[i];
                    ++i;
                }
                name[i] = '\0';
            }
        }
    };

    struct NonSerialized {};

    struct AssetExtension {
        char ext[32]{};

        constexpr AssetExtension() = default;
        constexpr explicit AssetExtension(const char* str) {
            if (str) {
                int i = 0;
                while (str[i] && i < 31) {
                    ext[i] = str[i];
                    ++i;
                }
                ext[i] = '\0';
            }
        }
    };

    struct SavesToBinary {
        bool value = true;
    };

    struct ShaderSuffix {
        char suffix[16]{};

        constexpr ShaderSuffix() = default;
        constexpr explicit ShaderSuffix(const char* str) {
            if (str) {
                int i = 0;
                while (str[i] && i < 15) {
                    suffix[i] = str[i];
                    ++i;
                }
                suffix[i] = '\0';
            }
        }
    };

    // ==========================================
    // MetaInfoArray & Reflection Primitives
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

    template <typename Enum>
    consteval auto enumerator_count() {
        return std::meta::enumerators_of(^^Enum).size();
    }

    template <typename Enum>
    consteval auto get_enumerators_array() {
        auto vec = std::meta::enumerators_of(^^Enum);
        constexpr std::size_t N = enumerator_count<Enum>();
        MetaInfoArray<N> arr{};
        for (std::size_t i = 0; i < vec.size(); ++i) {
            arr.data[i] = vec[i];
        }
        return arr;
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

    template <typename Attr>
    consteval bool has_annotation(std::meta::info entity) {
        auto annots = std::meta::annotations_of_with_type(entity, ^^Attr);
        return !annots.empty();
    }

    template <typename Attr>
    consteval std::optional<Attr> get_annotation(std::meta::info entity) {
        auto annots = std::meta::annotations_of_with_type(entity, ^^Attr);
        if (!annots.empty()) {
            return std::meta::extract<Attr>(annots[0]);
        }
        return std::nullopt;
    }

    template <std::meta::info mem>
    consteval std::string_view get_json_member_name() {
        constexpr auto jsonNameAttr = get_annotation<JsonName>(mem);
        if constexpr (jsonNameAttr.has_value()) {
            return jsonNameAttr->name;
        } else {
            return std::meta::identifier_of(mem);
        }
    }

    // ==========================================
    // Generic Enum Reflection
    // ==========================================
    template <typename Enum>
    constexpr std::string_view EnumToString(Enum value) {
        static constexpr auto enums = get_enumerators_array<Enum>();
        template for (constexpr auto e : enums) {
            if (value == [:e:]) {
                return std::meta::identifier_of(e);
            }
        }
        return "Unknown";
    }

    template <typename Enum>
    constexpr std::optional<Enum> StringToEnum(std::string_view name) {
        static constexpr auto enums = get_enumerators_array<Enum>();
        template for (constexpr auto e : enums) {
            if (name == std::meta::identifier_of(e)) {
                return [:e:];
            }
        }
        return std::nullopt;
    }

    // ==========================================
    // Generic JSON Serialization & Deserialization
    // ==========================================
    template <typename T>
    void SerializeValueToJson(const T& val, rapidjson::Value& outVal, rapidjson::Document::AllocatorType& allocator);

    template <typename T>
    bool DeserializeValueFromJson(T& val, const rapidjson::Value& inVal, AssetManagerInterface* assetManager = nullptr);

    template <typename T>
    void SerializeAssetData(const T& data, rapidjson::Value& outObj, rapidjson::Document::AllocatorType& allocator) {
        if (!outObj.IsObject()) {
            outObj.SetObject();
        }
        static constexpr auto members = get_members_array<T>();
        template for (constexpr auto mem : members) {
            if constexpr (!has_annotation<NonSerialized>(mem)) {
                constexpr auto jsonOpt = get_annotation<JsonName>(mem);
                constexpr auto idName = std::meta::identifier_of(mem);
                const char* fieldName = jsonOpt.has_value() ? jsonOpt->name : idName.data();

                const auto& memberVal = data.[:mem:];
                using MemberType = std::decay_t<decltype(memberVal)>;

                if constexpr (std::is_same_v<MemberType, std::shared_ptr<AssetInfo>>) {
                    if (memberVal) {
                        std::string uuidStr = boost::uuids::to_string(GetAssetInfoId(memberVal));
                        rapidjson::Value key(fieldName, allocator);
                        rapidjson::Value val(uuidStr.c_str(), static_cast<rapidjson::SizeType>(uuidStr.size()), allocator);
                        outObj.AddMember(key, val, allocator);
                    }
                } else {
                    rapidjson::Value key(fieldName, allocator);
                    rapidjson::Value fieldVal;
                    SerializeValueToJson(memberVal, fieldVal, allocator);
                    outObj.AddMember(key, fieldVal, allocator);
                }
            }
        }
    }

    template <typename T>
    bool DeserializeAssetData(T& data, const rapidjson::Value& inObj, AssetManagerInterface* assetManager = nullptr) {
        if (!inObj.IsObject()) return false;
        static constexpr auto members = get_members_array<T>();
        template for (constexpr auto mem : members) {
            if constexpr (!has_annotation<NonSerialized>(mem)) {
                constexpr auto jsonOpt = get_annotation<JsonName>(mem);
                constexpr auto idName = std::meta::identifier_of(mem);

                const char* keyToFind = nullptr;
                if constexpr (jsonOpt.has_value()) {
                    if (inObj.HasMember(jsonOpt->name)) {
                        keyToFind = jsonOpt->name;
                    }
                }
                if (!keyToFind && inObj.HasMember(idName.data())) {
                    keyToFind = idName.data();
                }

                if (keyToFind) {
                    const auto& fieldVal = inObj[keyToFind];
                    DeserializeValueFromJson(data.[:mem:], fieldVal, assetManager);
                }
            }
        }
        return true;
    }

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
        } else if constexpr (std::is_same_v<RawT, int64_t>) {
            outVal.SetInt64(val);
        } else if constexpr (std::is_same_v<RawT, uint64_t> || std::is_same_v<RawT, size_t>) {
            outVal.SetUint64(static_cast<uint64_t>(val));
        } else if constexpr (std::is_same_v<RawT, bool>) {
            outVal.SetBool(val);
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
        } else if constexpr (std::is_same_v<RawT, std::shared_ptr<AssetInfo>>) {
            if (val) {
                std::string str = boost::uuids::to_string(GetAssetInfoId(val));
                outVal.SetString(str.c_str(), static_cast<rapidjson::SizeType>(str.size()), allocator);
            } else {
                outVal.SetNull();
            }
        } else if constexpr (std::is_enum_v<RawT>) {
            std::string_view name = EnumToString(val);
            if (name != "Unknown") {
                outVal.SetString(name.data(), static_cast<rapidjson::SizeType>(name.size()), allocator);
            } else {
                outVal.SetInt(static_cast<int>(val));
            }
        } else if constexpr (requires { val.begin(); val.end(); } && !std::is_same_v<RawT, std::string>) {
            outVal.SetArray();
            for (const auto& item : val) {
                rapidjson::Value itemVal;
                SerializeValueToJson(item, itemVal, allocator);
                outVal.PushBack(itemVal, allocator);
            }
        } else if constexpr (std::is_class_v<RawT>) {
            SerializeAssetData(val, outVal, allocator);
        }
    }

    template <typename T>
    bool DeserializeValueFromJson(T& val, const rapidjson::Value& inVal, AssetManagerInterface* assetManager) {
        using RawT = std::decay_t<T>;
        if constexpr (std::is_same_v<RawT, float>) {
            if (inVal.IsNumber()) { val = inVal.GetFloat(); return true; }
        } else if constexpr (std::is_same_v<RawT, double>) {
            if (inVal.IsNumber()) { val = inVal.GetDouble(); return true; }
        } else if constexpr (std::is_same_v<RawT, int>) {
            if (inVal.IsInt()) { val = inVal.GetInt(); return true; }
        } else if constexpr (std::is_same_v<RawT, unsigned int>) {
            if (inVal.IsUint()) { val = inVal.GetUint(); return true; }
            if (inVal.IsInt()) { val = static_cast<unsigned int>(inVal.GetInt()); return true; }
        } else if constexpr (std::is_same_v<RawT, int64_t>) {
            if (inVal.IsInt64()) { val = inVal.GetInt64(); return true; }
        } else if constexpr (std::is_same_v<RawT, uint64_t> || std::is_same_v<RawT, size_t>) {
            if (inVal.IsUint64()) { val = inVal.GetUint64(); return true; }
            if (inVal.IsInt64()) { val = static_cast<uint64_t>(inVal.GetInt64()); return true; }
        } else if constexpr (std::is_same_v<RawT, bool>) {
            if (inVal.IsBool()) { val = inVal.GetBool(); return true; }
        } else if constexpr (std::is_same_v<RawT, std::string>) {
            if (inVal.IsString()) { val = inVal.GetString(); return true; }
        } else if constexpr (std::is_same_v<RawT, boost::uuids::uuid>) {
            if (inVal.IsString()) {
                try {
                    val = boost::uuids::string_generator()(inVal.GetString());
                    return true;
                } catch (...) { return false; }
            }
        } else if constexpr (std::is_same_v<RawT, glm::vec2>) {
            if (inVal.IsArray() && inVal.Size() >= 2) {
                val.x = inVal[0].GetFloat();
                val.y = inVal[1].GetFloat();
                return true;
            }
        } else if constexpr (std::is_same_v<RawT, glm::vec3>) {
            if (inVal.IsArray() && inVal.Size() >= 3) {
                val.x = inVal[0].GetFloat();
                val.y = inVal[1].GetFloat();
                val.z = inVal[2].GetFloat();
                return true;
            }
        } else if constexpr (std::is_same_v<RawT, glm::vec4>) {
            if (inVal.IsArray() && inVal.Size() >= 4) {
                val.x = inVal[0].GetFloat();
                val.y = inVal[1].GetFloat();
                val.z = inVal[2].GetFloat();
                val.w = inVal[3].GetFloat();
                return true;
            }
        } else if constexpr (std::is_same_v<RawT, glm::quat>) {
            if (inVal.IsArray() && inVal.Size() >= 4) {
                val.w = inVal[0].GetFloat();
                val.x = inVal[1].GetFloat();
                val.y = inVal[2].GetFloat();
                val.z = inVal[3].GetFloat();
                return true;
            }
        } else if constexpr (std::is_same_v<RawT, std::shared_ptr<AssetInfo>>) {
            if (inVal.IsString()) {
                std::string uuidStr = inVal.GetString();
                try {
                    boost::uuids::uuid id = boost::uuids::string_generator()(uuidStr);
                    val = ResolveAssetInfo(id, assetManager);
                    return val != nullptr;
                } catch (...) {
                    val = nullptr;
                    return false;
                }
            }
        } else if constexpr (std::is_enum_v<RawT>) {
            if (inVal.IsString()) {
                auto parsed = StringToEnum<RawT>(inVal.GetString());
                if (parsed.has_value()) {
                    val = parsed.value();
                    return true;
                }
            } else if (inVal.IsInt()) {
                val = static_cast<RawT>(inVal.GetInt());
                return true;
            }
        } else if constexpr (requires { val.resize(0); } && !std::is_same_v<RawT, std::string>) {
            if (inVal.IsArray()) {
                val.clear();
                val.resize(inVal.Size());
                for (rapidjson::SizeType i = 0; i < inVal.Size(); ++i) {
                    DeserializeValueFromJson(val[i], inVal[i], assetManager);
                }
                return true;
            }
        } else if constexpr (std::is_class_v<RawT>) {
            return DeserializeAssetData(val, inVal, assetManager);
        }
        return false;
    }

    // ==========================================
    // Automatic Content Hashing & Dependency Traversal
    // ==========================================
    template <typename T>
    size_t CalculateReflectedContentHash(const T& data) {
        size_t hash = 0;
        static constexpr auto members = get_members_array<T>();
        template for (constexpr auto mem : members) {
            if constexpr (!has_annotation<NonSerialized>(mem)) {
                const auto& memberVal = data.[:mem:];
                using MemberType = std::decay_t<decltype(memberVal)>;

                if constexpr (std::is_same_v<MemberType, std::shared_ptr<AssetInfo>>) {
                    if (memberVal) {
                        hash ^= GetAssetInfoContentHash(memberVal) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
                    }
                } else if constexpr (std::is_fundamental_v<MemberType> || std::is_enum_v<MemberType>) {
                    hash ^= std::hash<MemberType>{}(memberVal) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
                } else if constexpr (std::is_same_v<MemberType, std::string>) {
                    hash ^= std::hash<std::string>{}(memberVal) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
                } else if constexpr (std::is_same_v<MemberType, boost::uuids::uuid>) {
                    hash ^= boost::hash<boost::uuids::uuid>{}(memberVal) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
                } else if constexpr (std::is_same_v<MemberType, glm::vec2>) {
                    hash ^= std::hash<float>{}(memberVal.x) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
                    hash ^= std::hash<float>{}(memberVal.y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
                } else if constexpr (std::is_same_v<MemberType, glm::vec3>) {
                    hash ^= std::hash<float>{}(memberVal.x) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
                    hash ^= std::hash<float>{}(memberVal.y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
                    hash ^= std::hash<float>{}(memberVal.z) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
                } else if constexpr (std::is_same_v<MemberType, glm::vec4>) {
                    hash ^= std::hash<float>{}(memberVal.x) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
                    hash ^= std::hash<float>{}(memberVal.y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
                    hash ^= std::hash<float>{}(memberVal.z) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
                    hash ^= std::hash<float>{}(memberVal.w) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
                } else if constexpr (std::is_class_v<MemberType>) {
                    hash ^= CalculateReflectedContentHash(memberVal) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
                }
            }
        }
        return hash;
    }

    // ==========================================
    // Type-Safe Asset Access Traits & Concepts
    // ==========================================
    template <typename T>
    struct AssetDataTypeTrait {
        using type = void;
    };

    template <typename T>
        requires requires { typename T::DataType; }
    struct AssetDataTypeTrait<T> {
        using type = typename T::DataType;
    };

    template <typename T>
    using AssetDataType = typename AssetDataTypeTrait<T>::type;

} // namespace am

#endif // REASONABLEVULKAN_ASSETREFLECTION_HPP
