//
// Created by redkc on 09.05.2025.
//

#ifndef ASSETFACTORYREGISTRY_HPP
#define ASSETFACTORYREGISTRY_HPP

#include "AssetManager.hpp"

namespace ae {
    
class AssetFactoryRegistry {
public:
template<typename T>
        struct Registrar {
        explicit Registrar(ae::AssetType type) {
            ae::AssetManager::getInstance().registerFactory(type,
                [](ae::AssetFactoryData& factoryData) {
                    return std::make_shared<T>(factoryData);
                }
            );
        }
    };
};
}

#endif //ASSETFACTORYREGISTRY_HPP
