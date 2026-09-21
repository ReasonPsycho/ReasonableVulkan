#pragma once
#include "DescriptorManager.h"

// Template getOrLoadResource for backward-compatibility
template <typename T>
T* DescriptorManager::getOrLoadResource(const boost::uuids::uuid& assetId)
{
    if constexpr (std::is_same_v<T, ModelDescriptor>) {
        return getModel(getOrLoadModel(assetId));
    } else if constexpr (std::is_same_v<T, ShaderProgramDescriptor>) {
        return getShaderProgram(getOrLoadShaderProgram(assetId));
    } else if constexpr (std::is_same_v<T, ShaderDescriptor>) {
        return getShader(getOrLoadShader(assetId));
    } else if constexpr (std::is_same_v<T, TextureDescriptor>) {
        return getTexture(getOrLoadTexture(assetId));
    } else if constexpr (std::is_same_v<T, MaterialDescriptor>) {
        return getMaterial(getOrLoadMaterial(assetId));
    } else if constexpr (std::is_same_v<T, MeshDescriptor>) {
        return getMesh(getOrLoadMesh(assetId));
    } else {
        static_assert(!sizeof(T*), "Unsupported descriptor type");
    }
}

template <typename T>
T* DescriptorManager::getOrLoadResource(std::string lookUpName)
{
    auto id = assetManager->getAssetUuid(lookUpName);
    if (id.has_value())
    {
        return getOrLoadResource<T>(id.value());
    } else {
        spdlog::error("Asset not found: {}", lookUpName);
        throw std::runtime_error("Asset not found");
    }
}
