//
// Created by redkc on 10/08/2025.
//

#ifndef RENDERDATA_H
#define RENDERDATA_H
#include <vulkan/vulkan_core.h>
#include "../../vks/src/base/VulkanDevice.h"

namespace vks
{

    class IVulkanDescriptor {
    public:
        IVulkanDescriptor(vks::base::VulkanDevice& device, VkQueue copyQueue)
            : device(device), copyQueue(copyQueue) {}
        virtual ~IVulkanDescriptor() = default;

        virtual void cleanup(){};
    protected:
        vks::base::VulkanDevice device;
        VkQueue copyQueue;
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
