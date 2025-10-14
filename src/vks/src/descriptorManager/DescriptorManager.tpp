#pragma once
#include "DescriptorManager.h"

template <typename T>
T* vks::DescriptorManager::getOrLoadResource(const boost::uuids::uuid& assetId)
{
    if (isResourceLoaded(assetId))
        return dynamic_cast<T*>(loadedResources[assetId].get());

    return dynamic_cast<T*>(loadResource(assetId));
}

template <typename T>
T* vks::DescriptorManager::getOrLoadResource(std::string path)
{
    auto assetInfo = assetManager->registerAsset(path);
    return dynamic_cast<T*>(loadResource(assetInfo->get()->id));
}

inline vks::IVulkanDescriptor* vks::DescriptorManager::loadResource(const boost::uuids::uuid& assetId)
{
    if (isResourceLoaded(assetId))
    {
         return dynamic_cast<IVulkanDescriptor*>(loadedResources[assetId].get());
    }

    auto assetInfo = assetManager->getAssetInfo(assetId);
    if (assetInfo.has_value())
    {
        auto assetPtr = assetInfo->get()->getAsset();
        switch (assetPtr->getType())
        {
        case am::AssetType::Mesh:
            {
                auto mesh = std::make_unique<MeshDescriptor>(this,
                                                         *assetPtr->getAssetDataAs<am::MeshData>(), glm::mat4(1),
                                                        *context);
                loadedResources[assetId] = std::move(mesh);
                return loadedResources[assetId].get();
                break;
            }

        case am::AssetType::Model:
            {
                auto model = std::make_unique<vks::ModelDescriptor>(this, *assetPtr->getAssetDataAs<am::ModelData>(),
                                                          *context);
                loadedResources[assetId] = std::move(model);
                return loadedResources[assetId].get();
                break;
            }

        case am::AssetType::Texture:
            {
                auto texture = std::make_unique<TextureDescriptor>(
                    *assetPtr->getAssetDataAs<am::TextureData>(),defaultSampler,*context);
                loadedResources[assetId] = std::move(texture);
                return loadedResources[assetId].get();
                break;
            }

        case am::AssetType::Material:
            {
                auto material = std::make_unique<MaterialDescriptor>(this,
                    *assetPtr->getAssetDataAs<am::MaterialData>(),
                   *context);
                loadedResources[assetId] = std::move(material);
                return loadedResources[assetId].get();
                break;
            }

        case am::AssetType::Shader:
            {
                auto shader = std::make_unique<ShaderDescriptor>(
                    *assetPtr->getAssetDataAs<am::ShaderData>(),*context);
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
        }
    }
    else
    {
        throw std::runtime_error("Asset not found");
    }

    return nullptr; // This should never be reached due to exceptions above
}
