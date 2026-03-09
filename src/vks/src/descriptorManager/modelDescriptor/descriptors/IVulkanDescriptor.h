//
// Created by redkc on 10/08/2025.
//

#ifndef RENDERDATA_H
#define RENDERDATA_H
#include <vulkan/vulkan_core.h>
#include <boost/uuid/uuid.hpp>
#include "../../vks/src/vulkanContext/VulkanContext.hpp"
namespace vks
{

    class IVulkanDescriptor {
    public:
        IVulkanDescriptor(const boost::uuids::uuid& assetId, VulkanContext& vulkanContext)
            : assetId(assetId), device(vulkanContext.getDevice()) {}
        virtual ~IVulkanDescriptor() = default;

        virtual void cleanup(){};

        const boost::uuids::uuid& getAssetId() const { return assetId; }
    protected:
        boost::uuids::uuid assetId;
        VkDevice device; // Used for cleanup
    };


    enum class VulkanHandleType {
        Mesh,
        Texture,
        Material,
        Vertex,
        Node
    };

}

#endif //RENDERDATA_H
