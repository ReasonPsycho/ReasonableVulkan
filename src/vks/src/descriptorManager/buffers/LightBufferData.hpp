//
// Created by redkc on 23/12/2025.
//

#ifndef REASONABLEVULKAN_LIGHTBUFFER_HPP
#define REASONABLEVULKAN_LIGHTBUFFER_HPP

#include <glm/vec3.hpp>
#include <glm/detail/type_mat4x4.hpp>
#include <vulkan/vulkan.h>

namespace vks
{
    struct LightsInfoUBO {
        struct UniformBlock {
            alignas(4) int32_t directionalLightCount;
            alignas(4) int32_t pointLightCount;
            alignas(4) int32_t spotLightCount;
            alignas(4) float far_plane;
        } uniformBlock;

        struct {
            VkBuffer buffer;
            VkDeviceMemory memory;
            VkDescriptorSet descriptorSet;
            VkDescriptorBufferInfo descriptor;
            void* mapped = nullptr;
        } buffer;
    };

    // Directional light rendering data
    struct DirectionalLightBufferData
    {
        alignas(16) glm::vec3 direction;      // World-space direction vector
        alignas(4)  float intensity;
        alignas(16) glm::vec3 color;
        alignas(4)  float shadowBias;         // Bias for shadow mapping
        alignas(16) glm::mat4 lightSpaceMatrix;
        alignas(4)  float shadowStrength;     // Strength/intensity of the shadow
        alignas(4)  int32_t shadowMapIndex;   // Index into the shadow map array
        alignas(4)  bool castShadows;         // Whether to cast shadows
        alignas(4)  float padding;
    };

    // Point light rendering data
    struct PointLightBufferData
    {
        alignas(16) glm::vec3 position;       // World-space position
        alignas(4)  float intensity;
        alignas(16) glm::vec3 color;
        alignas(4)  float radius;             // Light influence radius
        alignas(4)  float falloff;            // Falloff/attenuation factor
        alignas(4)  float shadowBias;         // Bias for shadow mapping
        alignas(4)  float shadowStrength;     // Strength/intensity of the shadow
        alignas(4)  int32_t shadowMapIndex;   // Index into the cube shadow map array
        alignas(4)  bool castShadows;
        alignas(16) glm::mat4 lightSpaceMatrices[6];
        alignas(4)  float padding;         // Alignment padding for GPU buffers
    };

    // Spot light rendering data
    struct SpotLightBufferData
    {
        alignas(16) glm::vec3 position;       // World-space position
        alignas(4)  float innerAngle;         // Inner cone angle (in degrees)
        alignas(16) glm::vec3 direction;      // World-space direction vector
        alignas(4)  float outerAngle;         // Outer cone angle (in degrees)
        alignas(16) glm::vec3 color;
        alignas(4)  float range;              // Maximum light range
        alignas(16) glm::mat4 lightSpaceMatrix;
        alignas(4)  float intensity;
        alignas(4)  float shadowBias;         // Bias for shadow mapping
        alignas(4)  float shadowStrength;     // Strength/intensity of the shadow
        alignas(4)  int32_t shadowMapIndex;   // Index into the shadow map array
        alignas(4)  bool castShadows;         // Whether to cast shadows
        alignas(4)  float padding;
    };
}

#endif