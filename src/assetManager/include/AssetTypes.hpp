#ifndef ASSETTYPES_HPP
#define ASSETTYPES_HPP

#include <ostream>
#include <string>
#include <sstream>
#include <filesystem>

#include "assetDatas/ShaderData.h"


namespace am {
    enum class AssetFormat {
        Json,
        Binary
    };

    enum class AssetType {
        Mesh,
        Model,
        Texture,
        Shader,
        ShaderProgram,
        Animation,
        Material,
        Animator,
        Scene,
        Prefab,
        Other // Just for testing
    };

    inline std::ostream &operator<<(std::ostream &os, const AssetType &type) {
        switch (type) {
            case AssetType::Mesh:
                return os << "Mesh";
            case AssetType::Model:
                return os << "Model";
            case AssetType::Texture:
                return os << "Texture";
            case AssetType::Shader:
                return os << "Shader";
            case AssetType::ShaderProgram:
                return os << "ShaderProgram";
            case AssetType::Animation:
                return os << "Animation";
            case AssetType::Animator:
                return os << "Animator";
            case AssetType::Other:
                return os << "Other";
            case AssetType::Material:
                return os << "Material";
            case AssetType::Scene:
                return os << "Scene";
            case AssetType::Prefab:
                return os << "Prefab";
            default:
                return os << "Unknown";
        }
    }

    inline std::string AssetTypeToString(AssetType type) {
        std::stringstream ss;
        ss << type;
        return ss.str();
    }

    inline AssetType StringToAssetType(const std::string& str) {
        if (str == "Mesh") return AssetType::Mesh;
        if (str == "Model") return AssetType::Model;
        if (str == "Texture") return AssetType::Texture;
        if (str == "Shader") return AssetType::Shader;
        if (str == "ShaderProgram") return AssetType::ShaderProgram;
        if (str == "Animation") return AssetType::Animation;
        if (str == "Material") return AssetType::Material;
        if (str == "Animator") return AssetType::Animator;
        if (str == "Scene") return AssetType::Scene;
        if (str == "Prefab") return AssetType::Prefab;
        return AssetType::Other;
    }

    inline AssetType GetAssetTypeFromExtension(const std::string& extension) {

        std::string ext = extension; // make a copy

        // Strip prefixes
        size_t lastUnderscore = ext.find_last_of('_');
        if (lastUnderscore != std::string::npos) {
            ext = "." + ext.substr(lastUnderscore + 1);
        }

        if (ext == ".fbx")       return AssetType::Model;
        if (ext == ".png")       return AssetType::Texture;
        if (ext == ".spv")       return AssetType::Shader;
        if (ext == ".spdv")      return AssetType::Shader;
        if (ext == ".frag")      return AssetType::Shader;
        if (ext == ".vert")      return AssetType::Shader;
        if (ext == ".geom")      return AssetType::Shader;
        if (ext == ".shaderImport") return AssetType::ShaderProgram;
        if (ext == ".shader")    return AssetType::ShaderProgram;
        if (ext == ".model")     return AssetType::Model;
        if (ext == ".material")  return AssetType::Material;
        if (ext == ".mesh")      return AssetType::Mesh;
        if (ext == ".texture")   return AssetType::Texture;
        if (ext == ".scene")     return AssetType::Scene;
        if (ext == ".prefab")    return AssetType::Prefab;

        return AssetType::Other;
    }

    inline std::string GetExtensionFromAssetType(AssetType type) {
        switch (type) {
            case AssetType::Mesh:          return ".mesh";
            case AssetType::Model:         return ".model";
            case AssetType::Texture:       return ".texture";
            case AssetType::Shader:        return ".shader";
            case AssetType::ShaderProgram: return ".shaderprogram";
            case AssetType::Animation:     return ".animation";
            case AssetType::Material:      return ".material";
            case AssetType::Animator:      return ".animator";
            case AssetType::Scene:         return ".scene";
            case AssetType::Prefab:        return ".prefab";
            default:                       return ".other";
        }
    }

    inline bool GetEditorSavesToBin(AssetType type) {
        switch (type) {
        case AssetType::Mesh:          return true;
        case AssetType::Model:         return false;
        case AssetType::Texture:       return true;
        case AssetType::Shader:        return true;
        case AssetType::ShaderProgram: return false;
        case AssetType::Animation:     return true;
        case AssetType::Material:      return false;
        case AssetType::Animator:      return true;
        case AssetType::Scene:         return false;
        case AssetType::Prefab:        return false;
        default:                       return false;
        }
    }

    inline std::string GetShaderSufix(ShaderStage shaderStage)
    {
        switch (shaderStage)
        {
            case ShaderStage::Vertex: return "vs"; break;
            case ShaderStage::TessellationControl: return "tcs"; break;
            case ShaderStage::TessellationEvaluation: return "tes"; break;
            case ShaderStage::Fragment: return "fs"; break;
            case ShaderStage::Geometry: return "gs"; break;
            case ShaderStage::Compute: return "cs"; break;
            default: return "";
        }
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
