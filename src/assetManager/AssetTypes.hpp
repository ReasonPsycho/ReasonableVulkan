#ifndef ASSETTYPES_HPP
#define ASSETTYPES_HPP

#include <ostream>

namespace ae {
    enum class AssetType {
        Mesh,
        Model,
        Texture,
        Shader,
        Animation,
        Animator
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
            default:
                return os << "Unknown";
        }
    }
}

#endif //ASSETTYPES_HPP
