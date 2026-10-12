//
// Created by redkc on 10/08/2025.
//

#include "MaterialDescriptor.h"

#include "Asset.hpp"
#include "../../../DescriptorManager.h"

vks::MaterialDescriptor::MaterialDescriptor(const boost::uuids::uuid& assetId, DescriptorManager* assetHandleManager, am::MaterialData& materialData, VulkanContext& vulkanContext) : IVulkanDescriptor(assetId, vulkanContext)
{
    this->assetHandleManager = assetHandleManager;
    // Initialize numeric fields
    alphaCutoff = materialData.alphaCutoff;
    metallicFactor = materialData.metallicFactor;
    roughnessFactor = materialData.roughnessFactor;
    baseColorFactor = materialData.baseColorFactor;
    descriptorBindingFlags = 0;

    // Initialize textures (if valid AssetInfo is provided)
    if (materialData.baseColorTexture) {
        baseColorTexture = assetHandleManager->getOrLoadResource<TextureDescriptor>(materialData.baseColorTexture->id);
        if (baseColorTexture) {
            baseColorTexIndex = baseColorTexture->getSlotIndex();
            descriptorBindingFlags |= vks::DescriptorBindingFlags::ImageBaseColor;
        }
    }
    if (materialData.metallicRoughnessTexture) {
        metallicRoughnessTexture = assetHandleManager->getOrLoadResource<TextureDescriptor>(materialData.metallicRoughnessTexture->id);
        if (metallicRoughnessTexture) {
            metallicRoughnessTexIndex = metallicRoughnessTexture->getSlotIndex();
        }
    }
    if (materialData.normalTexture) {
        normalTexture = assetHandleManager->getOrLoadResource<TextureDescriptor>(materialData.normalTexture->id);
        if (normalTexture) {
            normalTexIndex = normalTexture->getSlotIndex();
            descriptorBindingFlags |= vks::DescriptorBindingFlags::ImageNormalMap;
        }
    }
    if (materialData.occlusionTexture) {
        occlusionTexture = assetHandleManager->getOrLoadResource<TextureDescriptor>(materialData.occlusionTexture->id);
        if (occlusionTexture) {
            occlusionTexIndex = occlusionTexture->getSlotIndex();
        }
    }
    if (materialData.emissiveTexture) {
        emissiveTexture = assetHandleManager->getOrLoadResource<TextureDescriptor>(materialData.emissiveTexture->id);
        if (emissiveTexture) {
            emissiveTexIndex = emissiveTexture->getSlotIndex();
        }
    }
    if (materialData.specularGlossinessTexture) {
        metallicRoughnessTexture = assetHandleManager->getOrLoadResource<TextureDescriptor>(materialData.specularGlossinessTexture->id);
        if (metallicRoughnessTexture) {
            metallicRoughnessTexIndex = metallicRoughnessTexture->getSlotIndex();
        }
    }

    if (assetHandleManager) {
        materialIndex = assetHandleManager->registerMaterial(this);
    }
}

vks::MaterialDescriptor::~MaterialDescriptor() {
}

vks::GPUMaterialData vks::MaterialDescriptor::getGPUMaterialData() const {
    GPUMaterialData data{};
    data.baseColorFactor = baseColorFactor;
    data.metallicFactor = metallicFactor;
    data.roughnessFactor = roughnessFactor;
    data.alphaCutoff = alphaCutoff;
    data.baseColorTexIndex = baseColorTexIndex;
    data.normalTexIndex = normalTexIndex;
    data.metallicRoughnessTexIndex = metallicRoughnessTexIndex;
    data.emissiveTexIndex = emissiveTexIndex;
    data.occlusionTexIndex = occlusionTexIndex;
    data.flags = descriptorBindingFlags;
    return data;
}

void vks::MaterialDescriptor::update(am::MaterialData& materialData) {
    alphaCutoff = materialData.alphaCutoff;
    metallicFactor = materialData.metallicFactor;
    roughnessFactor = materialData.roughnessFactor;
    baseColorFactor = materialData.baseColorFactor;
    descriptorBindingFlags = 0;

    baseColorTexture = nullptr;
    metallicRoughnessTexture = nullptr;
    normalTexture = nullptr;
    occlusionTexture = nullptr;
    emissiveTexture = nullptr;

    if (materialData.baseColorTexture) {
        baseColorTexture = assetHandleManager->getOrLoadResource<TextureDescriptor>(materialData.baseColorTexture->id);
        if (baseColorTexture) {
            baseColorTexIndex = baseColorTexture->getSlotIndex();
            descriptorBindingFlags |= vks::DescriptorBindingFlags::ImageBaseColor;
        }
    }
    if (materialData.metallicRoughnessTexture) {
        metallicRoughnessTexture = assetHandleManager->getOrLoadResource<TextureDescriptor>(materialData.metallicRoughnessTexture->id);
        if (metallicRoughnessTexture) {
            metallicRoughnessTexIndex = metallicRoughnessTexture->getSlotIndex();
        }
    }
    if (materialData.normalTexture) {
        normalTexture = assetHandleManager->getOrLoadResource<TextureDescriptor>(materialData.normalTexture->id);
        if (normalTexture) {
            normalTexIndex = normalTexture->getSlotIndex();
            descriptorBindingFlags |= vks::DescriptorBindingFlags::ImageNormalMap;
        }
    }
    if (materialData.occlusionTexture) {
        occlusionTexture = assetHandleManager->getOrLoadResource<TextureDescriptor>(materialData.occlusionTexture->id);
        if (occlusionTexture) {
            occlusionTexIndex = occlusionTexture->getSlotIndex();
        }
    }
    if (materialData.emissiveTexture) {
        emissiveTexture = assetHandleManager->getOrLoadResource<TextureDescriptor>(materialData.emissiveTexture->id);
        if (emissiveTexture) {
            emissiveTexIndex = emissiveTexture->getSlotIndex();
        }
    }
    if (materialData.specularGlossinessTexture) {
        metallicRoughnessTexture = assetHandleManager->getOrLoadResource<TextureDescriptor>(materialData.specularGlossinessTexture->id);
        if (metallicRoughnessTexture) {
            metallicRoughnessTexIndex = metallicRoughnessTexture->getSlotIndex();
        }
    }

    if (assetHandleManager) {
        assetHandleManager->updateMaterial(this);
    }
}