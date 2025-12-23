//
// Created by redkc on 23/12/2025.
// Light rendering data structures for the graphics engine
//

#ifndef REASONABLEVULKAN_LIGHTDATA_HPP
#define REASONABLEVULKAN_LIGHTDATA_HPP

#include <glm/vec3.hpp>
#include <glm/detail/type_mat4x4.hpp>

namespace gfx
{
    // Directional light rendering data
    struct DirectionalLightData
    {
        float intensity;
        glm::vec3 color;
    };

    // Point light rendering data
    struct PointLightData
    {
        float intensity;
        glm::vec3 color;
        float radius;             // Light influence radius
        float falloff;            // Falloff/attenuation factor
    };

    // Spot light rendering data
    struct SpotLightData
    {
        float intensity;
        float innerAngle;         // Inner cone angle (in degrees)
        glm::vec3 color;
        float outerAngle;         // Outer cone angle (in degrees)
        float range;              // Maximum light range
    };
}

#endif //REASONABLEVULKAN_LIGHTDATA_HPP