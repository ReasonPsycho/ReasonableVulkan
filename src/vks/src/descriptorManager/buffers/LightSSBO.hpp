//
// Created by redkc on 23/12/2025.
//

#ifndef REASONABLEVULKAN_LIGHTSSBO_HPP
#define REASONABLEVULKAN_LIGHTSSBO_HPP

#include <vector>
#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>

namespace vks
{
    struct LightSSBO {
        struct Buffer {
            VkBuffer buffer;
            VkDeviceMemory memory;
            VkDescriptorBufferInfo descriptor;
            void* mapped;
        } buffer;

        VkDescriptorSet descriptorSet;
    };
}

#endif //REASONABLEVULKAN_LIGHTSSBO_HPP