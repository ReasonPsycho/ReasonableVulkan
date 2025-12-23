//
// Created by redkc on 23/12/2025.
//

#ifndef REASONABLEVULKAN_LIGHTBUFFER_HPP
#define REASONABLEVULKAN_LIGHTBUFFER_HPP

#include <glm/vec3.hpp>
#include <glm/detail/type_mat4x4.hpp>

namespace vks
{
    // Directional light rendering data
    struct DirectionalLightBufferData
    {
        glm::vec3 direction;      // World-space direction vector
        float intensity;
        glm::vec3 color;
        float padding;            // Alignment padding for GPU buffers
    };

    // Point light rendering data
    struct PointLightBufferData
    {
        glm::vec3 position;       // World-space position
        float intensity;
        glm::vec3 color;
        float radius;             // Light influence radius
        float falloff;            // Falloff/attenuation factor
        float padding[3];         // Alignment padding for GPU buffers
    };

    // Spot light rendering data
    struct SpotLightBufferData
    {
        glm::vec3 position;       // World-space position
        float intensity;
        glm::vec3 direction;      // World-space direction vector
        float innerAngle;         // Inner cone angle (in degrees)
        glm::vec3 color;
        float outerAngle;         // Outer cone angle (in degrees)
        float range;              // Maximum light range
        float padding[3];         // Alignment padding for GPU buffers
    };
}

#endif