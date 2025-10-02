//
// Created by redkc on 10/08/2025.
//

#ifndef RENDERDATA_H
#define RENDERDATA_H
#include <vulkan/vulkan_core.h>
#include "../../vks/src/vulkanContext/VulkanContext.hpp"
namespace vks
{

    class IVulkanDescriptor {
    public:
        IVulkanDescriptor(VulkanContext& vulkanContext)
            : device(vulkanContext.getDevice()) {}
        virtual ~IVulkanDescriptor() = default;

        virtual void cleanup(){};
    protected:
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
