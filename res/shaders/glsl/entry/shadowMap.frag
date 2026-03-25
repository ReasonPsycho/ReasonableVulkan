#version 450
#extension GL_ARB_shading_language_include : enable

#include "../lighting/light_directional.glsl"
#include "../common/light_model_pc.glsl"
#include "../lighting/light_spot.glsl"

layout (location = 0) in vec4 inFragPos;

void main()
{
    vec3 lightPos;
    if (push_lm.lightType == 0) { // Directional
        lightPos = vec3(0.0); // Directional lights don't really have a position. TODO change so its x far away from the reverse vector of direction.
    } else { // Spot
        lightPos = spotLightSSBO.spotLights[push_lm.lightIndex].position;
    }

    // get distance between fragment and light source
    float lightDistance = length(inFragPos.xyz - lightPos);

    // map to [0;1] range by dividing by far_plane
    lightDistance = lightDistance / lights.far_plane;

    // write this as modified depth
    gl_FragDepth = lightDistance;
}