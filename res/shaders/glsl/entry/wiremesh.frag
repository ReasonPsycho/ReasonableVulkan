#version 450
#extension GL_ARB_shading_language_include : enable

#define WIREMESH_GLSL

#include "../common/scene_ubo.glsl"
#include "../common/vertex_io.glsl"

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec3 inWorldPos;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBitangent;

layout(location = 0) out vec4 outFragColor;

void main()
{
    outFragColor = vec4(inColor, 1.0);
}
