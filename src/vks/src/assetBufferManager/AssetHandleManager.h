//
// Created by redkc on 12/08/2025.
//

#ifndef ASSETBUFFERMANAGER_H
#define ASSETBUFFERMANAGER_H
#include <unordered_map>

#include "AssetManagerInterface.h"
#include "modelHandle/handles/IVulkanHandle.h"


class VulkanRenderer;

namespace vks
{

    class AssetHandleManager
    {
        // UUID to Handle mappings (deduplication)
        std::unordered_map<boost::uuids::uuid, std::unique_ptr<IVulkanHandle>> loadedResources;
        vks::IVulkanHandle* loadResource(const boost::uuids::uuid& assetId);
        am::AssetManagerInterface *assetManager;
        VulkanRenderer *vulkanRenderer;



    public:
        IVulkanHandle* getOrLoadResource(const boost::uuids::uuid& assetId);
        bool isResourceLoaded(const boost::uuids::uuid& assetId);

        AssetHandleManager() = default;
        ~AssetHandleManager() = default;
    };

}



#endif //ASSETBUFFERMANAGER_H
