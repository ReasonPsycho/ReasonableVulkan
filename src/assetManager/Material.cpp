//
// Created by redkc on 20.05.2025.
//

#include "Material.hpp"

size_t am::Material::calculateContentHash() const {
    size_t hash = 0;
    
    hash ^= diffuse->contentHash + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= specular->contentHash + 0x9e3779b9 + (hash << 6) + (hash >> 2);
   
    return hash;
}

am::AssetType am::Material::getType() const {
    return am::AssetType::Material;  
}
