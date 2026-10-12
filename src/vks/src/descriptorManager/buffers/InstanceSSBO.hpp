#ifndef REASONABLEVULKAN_INSTANCESSBO_HPP
#define REASONABLEVULKAN_INSTANCESSBO_HPP

#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>
#include <cstdint>

namespace vks
{
    struct alignas(16) GPUInstanceData {
        glm::mat4 modelMatrix{1.0f};
        uint32_t materialIndex{0};
        uint32_t customData{0};
        uint32_t padding[2]{0, 0};
    };

    struct InstanceSSBO {
        struct Buffer {
            VkBuffer buffer = VK_NULL_HANDLE;
            VkDeviceMemory memory = VK_NULL_HANDLE;
            VkDescriptorBufferInfo descriptor{};
            void* mapped = nullptr;
        } buffer;
    };
}

#endif //REASONABLEVULKAN_INSTANCESSBO_HPP
