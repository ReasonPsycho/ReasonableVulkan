#version 450
#extension GL_ARB_shading_language_include : enable

#include "../common/model_pc.glsl"
#include "../common/lightSpaceMatrix_pc.glsl"

layout (location = 0) in vec3 inPos;

void main()
{
    gl_Position = push_lsm.lightSpaceMatrix * push.model * vec4(inPos, 1.0);
}