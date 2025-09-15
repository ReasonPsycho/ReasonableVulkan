//
// Created by redkc on 12/08/2025.
//

#include "AssetHandleManager.h"

#include "Asset.hpp"
#include "AssetManagerInterface.h"
#include "../../VulkanRenderer.h"
#include "../../../assetManager/src/assets/ModelAsset.h"
#include "modelHandle/ModelHandle.h"
#include "modelHandle/handles/meshHandle/MeshHandle.h"
#include "modelHandle/handles/shaderHandle/ShaderHandle.h"
#include "modelHandle/handles/TextureHandle.h"


vks::IVulkanHandle* vks::AssetHandleManager::loadResource(const boost::uuids::uuid& assetId)
{
    if (loadedResources.find(assetId) != loadedResources.end())
    {
        return loadedResources[assetId].get();
    }

    std::optional<am::Asset*> asset = assetManager->getAsset(assetId);
    if (asset.has_value())
    {
        am::Asset* assetPtr = asset.value();
        switch (assetPtr->getType())
        {
        case am::AssetType::Mesh:
            auto mesh = std::make_unique<MeshHandle>(
                vulkanRenderer->vulkanDevice, *assetPtr->getAssetDataAs<am::MeshData>(), glm::mat4(1),
                vulkanRenderer->copyQueue);
                loadedResources[assetId] = std::move(mesh);
            return loadedResources[assetId].get();
            break;

            case am::AssetType::Model:
            auto model = std::make_unique<ModelHandle>(
                vulkanRenderer->vulkanDevice, *assetPtr->getAssetDataAs<am::ModelData>(),
                vulkanRenderer->copyQueue);
            loadedResources[assetId] = std::move(model);
            return loadedResources[assetId].get();
            break;

            case am::AssetType::Texture:
            auto texture = std::make_unique<TextureHandle>(
                vulkanRenderer->vulkanDevice, *assetPtr->getAssetDataAs<am::TextureData>(),
                vulkanRenderer->copyQueue);
            loadedResources[assetId] = std::move(texture);
            return loadedResources[assetId].get();
                break;

            case am::AssetType::Material:
            auto material = std::make_unique<MaterialHandle>(
                vulkanRenderer->vulkanDevice, *assetPtr->getAssetDataAs<am::MaterialData>(),
                vulkanRenderer->copyQueue);
            loadedResources[assetId] = std::move(material);
            return loadedResources[assetId].get();

            break;


            case am::AssetType::Shader:
            auto shader = std::make_unique<ShaderHandle>(
                vulkanRenderer->vulkanDevice, *assetPtr->getAssetDataAs<am::ShaderData>(),
                vulkanRenderer->copyQueue);
            loadedResources[assetId] = std::move(shader);
            return loadedResources[assetId].get();

            break;

            case am::AssetType::Animation:
                // Handle animation asset loading
                throw std::runtime_error("Animation loading not yet implemented");
                break;


            case am::AssetType::Animator:
                // Handle animator asset loading
                throw std::runtime_error("Animator loading not yet implemented");
                break;

            case am::AssetType::Other:
                // Handle other/generic asset loading
                throw std::runtime_error("Generic asset loading not yet implemented");
                break;

            default:
                throw std::runtime_error("Asset type not supported");
        }
    }
    else
    {
        throw std::runtime_error("Asset not found");
    }

    return nullptr; // This should never be reached due to exceptions above
}


vks::IVulkanHandle* vks::AssetHandleManager::getOrLoadResource(const boost::uuids::uuid& assetId)
{
    if (isResourceLoaded(assetId))
        return loadResource[assetId];

    return loadResource(assetId);
}

bool vks::AssetHandleManager::isResourceLoaded(const boost::uuids::uuid& assetId)
{
    return loadedResources.find(assetId) != loadedResources.end();
}
