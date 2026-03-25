#version 450
#extension GL_ARB_shading_language_include : enable

#include "../common/light_model_pc.glsl"
#include "../lighting/light_directional.glsl"
#include "../lighting/light_spot.glsl"

layout (location = 0) in vec3 inPos;

layout (location = 0) out vec4 outFragPos;

void main()
{
    outFragPos = push_lm.model * vec4(inPos, 1.0);
    mat4 lightSpaceMatrix;
    if (push_lm.lightType == 0) { // Directional
        lightSpaceMatrix = directionalLightSSBO.directionalLights[push_lm.lightIndex].lightSpaceMatrix;
    } else { // Spot
        lightSpaceMatrix = spotLightSSBO.spotLights[push_lm.lightIndex].lightSpaceMatrix;
    }
    gl_Position = lightSpaceMatrix * outFragPos;
}