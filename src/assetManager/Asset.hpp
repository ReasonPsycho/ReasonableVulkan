//
// Created by redkc on 09.05.2025.
//

#ifndef ASSET_HPP
#define ASSET_HPP
#include <string>
#include <boost/uuid/uuid.hpp>
#include <filesystem>

#include "AssetManager.hpp"
#include "UUIDManager.hpp"


namespace ae {
    enum class AssetType {
        Mesh,
        Model,
        Texture,
        Shader,
        Animation,
        Animator
    };

    class Asset {
    public:
        Asset(AssetManager) {
        }

        virtual ~Asset() = default;

        virtual void loadFromFile(const std::string &path) = 0;

        virtual AssetType getType() const = 0;
    };
}


#endif //ASSET_HPP
