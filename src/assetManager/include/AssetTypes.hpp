#ifndef ASSETTYPES_HPP
#define ASSETTYPES_HPP

#include <ostream>


namespace am {
    enum class AssetType {
        Mesh,
        Model,
        Texture,
        Shader,
        Animation,
        Material,
        Animator,
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
            case AssetType::Animation:
                return os << "Animation";
            case AssetType::Animator:
                return os << "Animator";
            case AssetType::Other:
                return os << "Other";
            case AssetType::Material:
                return os << "Material";
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
        if (str == "Animation") return AssetType::Animation;
        if (str == "Material") return AssetType::Material;
        if (str == "Animator") return AssetType::Animator;
        return AssetType::Other;
    }

    inline AssetType GetAssetTypeFromExtension(const std::string& extension) {
        if (extension == ".fbx")       return AssetType::Model;
        if (extension == ".png")       return AssetType::Texture;
        if (extension == ".spv")       return AssetType::Shader;
        if (extension == ".spdv")      return AssetType::Shader;
        if (extension == ".frag")      return AssetType::Shader;
        if (extension == ".vert")      return AssetType::Shader;
        return AssetType::Other;
    }
}


#endif //ASSETTYPES_HPP
