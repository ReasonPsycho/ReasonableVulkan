#version 450
#extension GL_ARB_shading_language_include : enable

#include "../common/light_model_pc.glsl"
#include "../lighting/light_point.glsl"

layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

layout (location = 0) out vec4 outFragPos; // FragPos from GS (output per emitvertex)

void main()
{
    for(int face = 0; face < 6; ++face)
    {
        gl_Layer = face; // built-in variable that specifies to which face we render.
        for(int i = 0; i < 3; ++i) // for each triangle vertex
        {
            outFragPos = gl_in[i].gl_Position;
            gl_Position = pointLightSSBO.pointLights[push_lm.lightIndex].lightSpaceMatrices[face] * outFragPos;
            EmitVertex();
        }
        EndPrimitive();
    }
}