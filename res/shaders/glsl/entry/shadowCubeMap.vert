#version 450
#extension GL_ARB_shading_language_include : enable

#include "../common/light_model_pc.glsl"

layout (location = 0) in vec3 inPos;

layout (location = 0) out vec4 outFragPos;

void main()
{
    outFragPos = push_lm.model * vec4(inPos, 1.0);
    gl_Position = outFragPos; // This is a placeholder, RenderManager likely handles setting up matrices for 6 faces
}
