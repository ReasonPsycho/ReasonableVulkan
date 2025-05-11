//
// Created by redkc on 09.05.2025.
//

#ifndef ASSETFACTORYREGISTRY_HPP
#define ASSETFACTORYREGISTRY_HPP



class AssetFactoryRegistry {
public:
    template<typename T>
    struct Registrar {
        explicit Registrar(ae::AssetType type) {
            ae::AssetManager::getInstance().registerFactory(type, 
                [](ae::BaseFactoryContext context) {
                    return std::make_shared<T>(context);
                }
            );
        }
    };
};

#endif //ASSETFACTORYREGISTRY_HPP
