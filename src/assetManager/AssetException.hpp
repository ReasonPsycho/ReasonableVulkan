#ifndef ASSETEXCEPTION_HPP
#define ASSETEXCEPTION_HPP

#include <stdexcept>
#include <string>

namespace am {
    class AssetException : public std::runtime_error {
    public:
        explicit AssetException(const std::string& message) 
            : std::runtime_error(message) {}
    };

    class AssetFactoryNotFoundException : public AssetException {
    public:
        explicit AssetFactoryNotFoundException(const std::string& assetType) 
            : AssetException("No factory registered for asset type: " + assetType) {}
    };
}

#endif //ASSETEXCEPTION_HPP