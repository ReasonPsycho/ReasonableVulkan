#version 450
#extension GL_ARB_shading_language_include : enable
#extension GL_EXT_multiview : enable

#include "../common/light_model_pc.glsl"
#include "../lighting/light_point.glsl"

#define ENABLE_MULTVIEW 1

layout (location = 0) in vec3 inPos;

layout (location = 0) out vec4 outFragPos;

void main()
{
    outFragPos = push_lm.model * vec4(inPos, 1.0);
    gl_Position = pointLightSSBO.pointLights[push_lm.lightIndex].lightSpaceMatrices[gl_ViewIndex] * outFragPos;
}
