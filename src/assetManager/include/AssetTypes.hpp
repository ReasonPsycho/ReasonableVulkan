#ifndef ASSETTYPES_HPP
#define ASSETTYPES_HPP

#include <ostream>
#include <string>
#include <sstream>
#include <filesystem>
#include "AssetReflection.hpp"
#include "assetDatas/ShaderData.h"

namespace am {
    enum class AssetFormat {
        Json,
        Binary
    };

    enum class AssetOwnership {
        Import,
        Managed,
        Unmanaged
    };

    enum class AssetType {
        Mesh [[=AssetExtension{".mesh"}, =SavesToBinary{true}]],
        Model [[=AssetExtension{".model"}, =SavesToBinary{false}]],
        Texture [[=AssetExtension{".texture"}, =SavesToBinary{true}]],
        Shader [[=AssetExtension{".shader"}, =SavesToBinary{true}]],
        ShaderProgram [[=AssetExtension{".shaderprogram"}, =SavesToBinary{false}]],
        Animation [[=AssetExtension{".animation"}, =SavesToBinary{true}]],
        Material [[=AssetExtension{".material"}, =SavesToBinary{false}]],
        Animator [[=AssetExtension{".animator"}, =SavesToBinary{true}]],
        Scene [[=AssetExtension{".scene"}, =SavesToBinary{false}]],
        Prefab [[=AssetExtension{".prefab"}, =SavesToBinary{false}]],
        Config [[=AssetExtension{".config"}, =SavesToBinary{false}]],
        Other [[=AssetExtension{".other"}, =SavesToBinary{false}]] // Just for testing
    };

    inline std::string AssetTypeToString(AssetType type) {
        std::string_view sv = EnumToString(type);
        return std::string(sv);
    }

    inline AssetType StringToAssetType(const std::string& str) {
        auto val = StringToEnum<AssetType>(str);
        return val.value_or(AssetType::Other);
    }

    inline std::ostream &operator<<(std::ostream &os, const AssetType &type) {
        return os << AssetTypeToString(type);
    }

    inline AssetOwnership StringToAssetOwnership(const std::string& str)
    {
        std::string ext = str; // make a copy

        // Strip prefixes
        size_t lastUnderscore = ext.find_last_of('_');
        if (lastUnderscore != std::string::npos) {
            ext = "." + ext.substr(lastUnderscore + 1);
        }
        
        if (ext == ".fbx" || ext == ".png" || ext == ".spv" || ext == ".spdv" ||
            ext == ".frag" || ext == ".vert" || ext == ".geom" || ext == ".shaderImport") {
            return AssetOwnership::Import;
        }

        static constexpr auto enums = get_enumerators_array<AssetType>();
        template for (constexpr auto e : enums) {
            constexpr auto annot = get_annotation<AssetExtension>(e);
            if constexpr (annot.has_value()) {
                if (ext == annot->ext) {
                    return AssetOwnership::Managed;
                }
            }
        }

        return AssetOwnership::Unmanaged;
    }
    
    inline AssetType GetAssetTypeFromExtension(const std::string& extension) {
        std::string ext = extension; // make a copy

        // Strip prefixes
        size_t lastUnderscore = ext.find_last_of('_');
        if (lastUnderscore != std::string::npos) {
            ext = "." + ext.substr(lastUnderscore + 1);
        }

        static constexpr auto enums = get_enumerators_array<AssetType>();
        template for (constexpr auto e : enums) {
            constexpr auto annot = get_annotation<AssetExtension>(e);
            if constexpr (annot.has_value()) {
                if (ext == annot->ext) {
                    return [:e:];
                }
            }
        }

        if (ext == ".fbx")          return AssetType::Model;
        if (ext == ".png")          return AssetType::Texture;
        if (ext == ".spv" || ext == ".spdv" || ext == ".frag" || ext == ".vert" || ext == ".geom") return AssetType::Shader;
        if (ext == ".shaderImport") return AssetType::ShaderProgram;

        return AssetType::Other;
    }

    inline std::string GetExtensionFromAssetType(AssetType type) {
        static constexpr auto enums = get_enumerators_array<AssetType>();
        template for (constexpr auto e : enums) {
            if (type == [:e:]) {
                constexpr auto annot = get_annotation<AssetExtension>(e);
                if constexpr (annot.has_value()) {
                    return annot->ext;
                }
            }
        }
        return ".other";
    }

    inline bool GetEditorSavesToBin(AssetType type) {
        static constexpr auto enums = get_enumerators_array<AssetType>();
        template for (constexpr auto e : enums) {
            if (type == [:e:]) {
                constexpr auto annot = get_annotation<SavesToBinary>(e);
                if constexpr (annot.has_value()) {
                    return annot->value;
                }
            }
        }
        return false;
    }

    inline std::string GetShaderSufix(ShaderStage shaderStage)
    {
        static constexpr auto enums = get_enumerators_array<ShaderStage>();
        template for (constexpr auto e : enums) {
            if (shaderStage == [:e:]) {
                constexpr auto annot = get_annotation<ShaderSuffix>(e);
                if constexpr (annot.has_value()) {
                    return annot->suffix;
                }
            }
        }
        return "";
    }

    inline std::string GetBinPath(const std::string& importPath, std::string additionalSufix) {
        std::filesystem::path p = std::filesystem::path(importPath).lexically_normal();
        std::string filename =  p.stem().string();
        if (additionalSufix != "")
        {
            filename += ".b_" + additionalSufix + "_" + p.extension().string().substr(1,p.extension().string().size()-1);
        }else
        {
            filename += ".b_" + p.extension().string().substr(1,p.extension().string().size()-1);
        }
        return (p.parent_path() / filename).string();
    }
}

#endif //ASSETTYPES_HPP
