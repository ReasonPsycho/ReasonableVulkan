//
// Created by redkc on 12/08/2025.
//

#ifndef ASSETBUFFERMANAGER_H
#define ASSETBUFFERMANAGER_H
#include <unordered_map>

#include "Asset.hpp"
#include "AssetManagerInterface.h"
#include "AssetTypes.hpp"
#include "modelHandle/ModelHandle.h"
#include "modelHandle/handles/IVulkanHandle.h"
#include "modelHandle/handles/meshHandle/MeshHandle.h"
#include "modelHandle/handles/shaderHandle/ShaderHandle.h"


class VulkanRenderer;

namespace vks
{

    class AssetHandleManager
    {
    public:
        template <typename T>
        T* getOrLoadResource(const boost::uuids::uuid& assetId);
        bool isResourceLoaded(const boost::uuids::uuid& assetId);

        AssetHandleManager() = default;
        ~AssetHandleManager() = default;

    private:
        am::AssetManagerInterface *assetManager;
        base::VulkanDevice *vulkanDevice;
	VkQueue copyQueue{VK_NULL_HANDLE};
        // UUID to Handle mappings (deduplication)
        std::unordered_map<boost::uuids::uuid, std::unique_ptr<IVulkanHandle>> loadedResources;
        vks::IVulkanHandle* loadResource(const boost::uuids::uuid& assetId){
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
        {
            auto mesh = std::make_unique<MeshHandle>(
                vulkanDevice, *assetPtr->getAssetDataAs<am::MeshData>(), glm::mat4(1),
                copyQueue);
            loadedResources[assetId] = std::move(mesh);
            return loadedResources[assetId].get();
            break;
        }

        case am::AssetType::Model: //TODO fix those meses of a calls since now vulkan device and copy queue ptr is in a asset manager
        {
            auto model = std::make_unique<ModelHandle>(*this,*assetPtr->getAssetDataAs<am::ModelData>(),vulkanDevice,copyQueue);
            loadedResources[assetId] = std::move(model);
            return loadedResources[assetId].get();
            break;
        }

        case am::AssetType::Texture:
        {
            auto texture = std::make_unique<TextureHandle>(
                *assetPtr->getAssetDataAs<am::TextureData>(),vulkanDevice,
                copyQueue);
            loadedResources[assetId] = std::move(texture);
            return loadedResources[assetId].get();
            break;
        }

        case am::AssetType::Material:
        {
            auto material = std::make_unique<MaterialHandle>(
                vulkanDevice, *assetPtr->getAssetDataAs<am::MaterialData>(),
                copyQueue);
            loadedResources[assetId] = std::move(material);
            return loadedResources[assetId].get();
            break;
        }

        case am::AssetType::Shader:
        {
            auto shader = std::make_unique<ShaderHandle>(
                 *assetPtr->getAssetDataAs<am::ShaderData>(),vulkanDevice,
                copyQueue);
            loadedResources[assetId] = std::move(shader);
            return loadedResources[assetId].get();
            break;
        }

        case am::AssetType::Animation:
        {
            // Handle animation asset loading
            throw std::runtime_error("Animation loading not yet implemented");
            break;
        }

        case am::AssetType::Animator:
        {
            // Handle animator asset loading
            throw std::runtime_error("Animator loading not yet implemented");
            break;
        }

        case am::AssetType::Other:
        {
            // Handle other/generic asset loading
            throw std::runtime_error("Generic asset loading not yet implemented");
            break;
        }

        default:
            throw std::runtime_error("Asset type not supported");
        }    }
    else
    {
        throw std::runtime_error("Asset not found");
    }

    return nullptr; // This should never be reached due to exceptions above
}

    };

}

#include "AssetHandleManager.tpp"

#endif //ASSETBUFFERMANAGER_H
