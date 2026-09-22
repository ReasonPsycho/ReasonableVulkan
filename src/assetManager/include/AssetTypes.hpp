#ifndef ASSETTYPES_HPP
#define ASSETTYPES_HPP

#include <ostream>
#include <string>
#include <sstream>
#include <filesystem>
#include "AssetReflection.hpp"
#include "assetDatas/ShaderData.h"
#include "IconsFontAwesome6.h"

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
        Mesh [[=AssetExtension{".mesh"}, =SavesToBinary{true}, =AssetIcon{ICON_FA_CUBE}]],
        Model [[=AssetExtension{".model"}, =SavesToBinary{false}, =AssetIcon{ICON_FA_CUBE}]],
        Texture [[=AssetExtension{".texture"}, =SavesToBinary{true}, =AssetIcon{ICON_FA_FILE_IMAGE}]],
        Shader [[=AssetExtension{".shader"}, =SavesToBinary{true}, =AssetIcon{ICON_FA_FILE_CODE}]],
        ShaderProgram [[=AssetExtension{".shaderprogram"}, =SavesToBinary{false}, =AssetIcon{ICON_FA_FILE_CODE}]],
        Animation [[=AssetExtension{".animation"}, =SavesToBinary{true}, =AssetIcon{ICON_FA_PERSON_RUNNING}]],
        Material [[=AssetExtension{".material"}, =SavesToBinary{false}, =AssetIcon{ICON_FA_PALETTE}]],
        Animator [[=AssetExtension{".animator"}, =SavesToBinary{true}, =AssetIcon{ICON_FA_PERSON_RUNNING}]],
        Scene [[=AssetExtension{".scene"}, =SavesToBinary{false}, =AssetIcon{ICON_FA_GLOBE}]],
        Prefab [[=AssetExtension{".prefab"}, =SavesToBinary{false}, =AssetIcon{ICON_FA_BOXES_STACKED}]],
        Config [[=AssetExtension{".config"}, =SavesToBinary{false}, =AssetIcon{ICON_FA_GEAR}]],
        Other [[=AssetExtension{".other"}, =SavesToBinary{false}, =AssetIcon{ICON_FA_FILE}]] // Just for testing
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
        
        if (ext == ".fbx" || ext == ".blend" || ext == ".blend1" || ext == ".obj" || ext == ".gltf" || ext == ".glb" ||
            ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga" || ext == ".dds" || ext == ".hdr" ||
            ext == ".spv" || ext == ".spdv" || ext == ".frag" || ext == ".vert" || ext == ".geom" || ext == ".comp" || ext == ".glsl" || ext == ".hlsl" ||
            ext == ".shaderImport") {
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

        if (ext == ".fbx" || ext == ".blend" || ext == ".blend1" || ext == ".obj" || ext == ".gltf" || ext == ".glb") return AssetType::Model;
        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga" || ext == ".dds" || ext == ".hdr") return AssetType::Texture;
        if (ext == ".spv" || ext == ".spdv" || ext == ".frag" || ext == ".vert" || ext == ".geom" || ext == ".comp" || ext == ".glsl" || ext == ".hlsl") return AssetType::Shader;
        if (ext == ".shaderImport") return AssetType::ShaderProgram;

        return AssetType::Other;
    }

    inline const char* GetIconFromAssetType(AssetType type) {
        static constexpr auto enums = get_enumerators_array<AssetType>();
        template for (constexpr auto e : enums) {
            if (type == [:e:]) {
                static constexpr auto annot = get_annotation<AssetIcon>(e);
                if constexpr (annot.has_value()) {
                    return annot->icon;
                }
            }
        }
        return ICON_FA_FILE;
    }

    inline const char* GetAssetIcon(const std::filesystem::path& path) {
        std::string ext = path.extension().string();

        if (ext == ".meta" || ext == ".json" || ext == ".txt" || ext == ".md" || ext == ".log") {
            return ICON_FA_FILE_LINES;
        }
        if (ext == ".fnt" || ext == ".ttf" || ext == ".otf") {
            return ICON_FA_FONT;
        }
        if (ext == ".wav" || ext == ".mp3" || ext == ".ogg" || ext == ".flac") {
            return ICON_FA_FILE_AUDIO;
        }

        AssetType type = GetAssetTypeFromExtension(ext);
        return GetIconFromAssetType(type);
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
