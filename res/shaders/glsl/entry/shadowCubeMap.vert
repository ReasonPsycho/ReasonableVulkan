#version 450
#extension GL_ARB_shading_language_include : enable

#include "../common/cubeShadow_pc.glsl"
#include "../common/model_pc.glsl"

layout (location = 0) in vec4 inFragPos;

void main()
{
       gl_Position = push_clsm.model * push.model * vec4(inPos, 1.0);
}  
