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
}

#endif //ASSETTYPES_HPP
