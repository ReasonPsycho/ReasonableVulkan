#ifndef ASSETFACTORYREGISTRY_HPP
#define ASSETFACTORYREGISTRY_HPP

#include <memory>
#include <expected>
#include <string>
#include "AssetManager.hpp"

namespace am {
    class AssetFactoryRegistry {
    public:
        template<typename T>
        struct Registrar {
            explicit Registrar(am::AssetType type) {
                am::AssetManager::getInstance().registerFactory(type,
                                                                [](am::AssetFactoryData &factoryData) -> std::expected<
                                                            std::unique_ptr<am::Asset>, std::exception_ptr> {
                                                                    try {
                                                                        auto asset = std::make_unique<T>(factoryData);
                                                                        return asset;
                                                                    } catch (...) {
                                                                        return std::unexpected(
                                                                            std::current_exception());
                                                                    }
                                                                }
                );
            }
        };
    };
}

#endif //ASSETFACTORYREGISTRY_HPP
