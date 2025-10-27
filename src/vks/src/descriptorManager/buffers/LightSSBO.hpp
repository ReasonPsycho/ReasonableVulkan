//
// Created by redkc on 25/10/2025.
//

#ifndef REASONABLEVULKAN_LIGHTSSSBO_HPP
#define REASONABLEVULKAN_LIGHTSSSBO_HPP

#include <glm/glm.hpp>

namespace vks {

    // Common light data structure aligned for GPU
    struct alignas(16) LightData {
        alignas(4) float r;
        alignas(4) float g;
        alignas(4) float b;
        alignas(4) float intensity;
    };

    // Specific light types for SSBO
    struct alignas(16) DirectionalLightSSBO {
        alignas(16) LightData base;
        alignas(4) float constant;
        alignas(4) float linear;
        alignas(4) float quadratic;
        alignas(4) float radius;
        alignas(4) float innerCutoff;
        alignas(4) float outerCutoff;
        alignas(8) float padding[2];  // Padding to maintain 16-byte alignment
    };

    struct alignas(16) PointLightSSBO {
        alignas(16) LightData base;
        alignas(4) float constant;
        alignas(4) float linear;
        alignas(4) float quadratic;
        alignas(4) float radius;
    };

    struct alignas(16) SpotLightSSBO {
        alignas(16) LightData base;
        alignas(16) glm::vec3 direction;
        alignas(4) float padding;  // Padding to maintain 16-byte alignment
    };

    // Light buffer for SSBO

    struct LightSSBO
    {
        struct alignas(16) LightBufferData {
            alignas(4) uint32_t directionalLightCount;
            alignas(4) uint32_t pointLightCount;
            alignas(4) uint32_t spotLightCount;
            alignas(4) uint32_t padding;
            DirectionalLightSSBO directionalLights[16];  // Adjust array size as needed
            PointLightSSBO pointLights[16];             // Adjust array size as needed
            SpotLightSSBO spotLights[16];              // Adjust array size as needed
        };

        struct {
            VkBuffer buffer;
            VkDeviceMemory memory;
            VkDescriptorSet descriptorSet;
            VkDescriptorBufferInfo descriptor;
            void* mapped = nullptr;
        } buffer;
    };
} // namespace vks

#endif //REASONABLEVULKAN_LIGHTSSSBO_HPP