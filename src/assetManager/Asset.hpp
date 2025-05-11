#ifndef ASSET_HPP
#define ASSET_HPP
#include <string>
#include <boost/uuid/uuid.hpp>
#include <filesystem>
#include "AssetTypes.hpp"
#include "AssetManager.hpp"

namespace ae {
    class Asset {
    public:
        explicit Asset(BaseFactoryContext baseFactoryContext) {}
        virtual ~Asset() = default;
        virtual size_t calculateContentHash() const = 0;  
        [[nodiscard]] virtual AssetType getType() const = 0;
    protected:
        std::string path;
        boost::uuids::uuid id;
    };
}

#endif //ASSET_HPP