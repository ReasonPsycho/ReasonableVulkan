#version 450
#extension GL_ARB_shading_language_include : enable

#include "../common/scene_ubo.glsl"

layout(binding = 1, set = 0) uniform samplerCube samplerCubeMap;

layout(location = 0) in vec3 inUVW;
layout(location = 0) out vec4 outFragColor;

void main()
{
    outFragColor = texture(samplerCubeMap, inUVW);
}
