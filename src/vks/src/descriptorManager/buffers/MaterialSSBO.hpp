#ifndef REASONABLEVULKAN_MATERIALSSBO_HPP
#define REASONABLEVULKAN_MATERIALSSBO_HPP

#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>
#include <cstdint>

namespace vks
{
    struct alignas(16) GPUMaterialData {
        glm::vec4 baseColorFactor{1.0f};
        float metallicFactor{1.0f};
        float roughnessFactor{1.0f};
        float alphaCutoff{1.0f};
        uint32_t baseColorTexIndex{0};
        uint32_t normalTexIndex{0};
        uint32_t metallicRoughnessTexIndex{0};
        uint32_t emissiveTexIndex{0};
        uint32_t occlusionTexIndex{0};
        uint32_t flags{0};
        uint32_t padding[3]{0, 0, 0};
    };

    struct MaterialSSBO {
        struct Buffer {
            VkBuffer buffer = VK_NULL_HANDLE;
            VkDeviceMemory memory = VK_NULL_HANDLE;
            VkDescriptorBufferInfo descriptor{};
            void* mapped = nullptr;
        } buffer;
    };
}

#endif //REASONABLEVULKAN_MATERIALSSBO_HPP
