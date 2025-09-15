//
// Created by redkc on 10/08/2025.
//

#ifndef RENDERDATA_H
#define RENDERDATA_H
#include <boost/uuid/uuid.hpp>
#include <vulkan/vulkan_core.h>

namespace am
{
    class Asset;
}

namespace vks
{
    namespace base
    {
        struct VulkanDevice;
    }

    class IVulkanHandle {

    public:
        IVulkanHandle(base::VulkanDevice*,VkQueue copyQueue) : device(device), copyQueue(copyQueue){};
        virtual ~IVulkanHandle() = default;
        virtual void cleanup() = 0;
    protected:
        base::VulkanDevice* device;
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
