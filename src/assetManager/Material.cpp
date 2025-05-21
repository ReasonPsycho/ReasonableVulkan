//
// Created by redkc on 20.05.2025.
//

#include "Material.hpp"

size_t am::Material::calculateContentHash() const {
    return 0;
}

am::AssetType am::Material::getType() const {
    return am::AssetType::Material;  
}
