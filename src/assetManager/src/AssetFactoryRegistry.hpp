#ifndef ASSETFACTORYREGISTRY_HPP
#define ASSETFACTORYREGISTRY_HPP

#include "AssetManager.hpp"

namespace am {
    class AssetFactoryRegistry {
    public:
        template<typename T>
        struct Registrar {
            explicit Registrar(am::AssetType type) {
                am::AssetManager::getInstance().registerFactory(type,
                                                                [](am::AssetFactoryData &factoryData) {
                                                                    return std::unique_ptr<T>(new T(factoryData));
                                                                }
                );
            }
        };
    };
}

#endif //ASSETFACTORYREGISTRY_HPP