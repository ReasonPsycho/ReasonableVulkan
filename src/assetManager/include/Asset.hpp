#ifndef ASSET_HPP
#define ASSET_HPP
#include <string>
#include <boost/uuid/uuid.hpp>
#include <filesystem>
#include "AssetTypes.hpp"
#include "../src/AssetManager.hpp"
#include "AssetInfo.hpp"

namespace am {
    class Asset {
    public:
        explicit Asset(AssetFactoryData assetFactoryData) {}

        virtual ~Asset() = default;

        virtual size_t calculateContentHash() const = 0;

        [[nodiscard]] virtual AssetType getType() const = 0;

        template<typename T>
          T* getAssetDataAs() {
            return static_cast<T*>(getAssetData());
        }

    protected:
        std::string path;
        boost::uuids::uuid id;

        virtual void* getAssetData() = 0;
    };
}

#endif //ASSET_HPP