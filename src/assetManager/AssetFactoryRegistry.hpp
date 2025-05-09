//
// Created by redkc on 09.05.2025.
//

#ifndef ASSETFACTORYREGISTRY_HPP
#define ASSETFACTORYREGISTRY_HPP


class AssetFactoryRegistry {
public:
    template<typename T>
    struct Registrar {
        Registrar(AssetType type) {
            AssetManager::getInstance().registerFactory(type, []() {
                return std::make_shared<T>();
            });
        }
    };
};

#endif //ASSETFACTORYREGISTRY_HPP
