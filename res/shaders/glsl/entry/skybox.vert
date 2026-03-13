#version 450
#extension GL_ARB_shading_language_include : enable

#include "../common/scene_ubo.glsl"
#include "../common/model_pc.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 0) out vec3 outUVW;

void main()
{
    outUVW = inPos;
    
    mat4 viewMat = sceneUbo.view;
    // HLSL matrix access [row][col]. In HLSL skybox.vert: viewMat[0][3] = 0.0 means first row, 4th column (translation x if row-major).
    // In GLSL column-major, viewMat[3][0] = 0.0 would be the same.
    // However, the intent is usually to strip translation.
    viewMat[3] = vec4(0.0, 0.0, 0.0, 1.0); 

    gl_Position = (sceneUbo.projection * viewMat * push.model * vec4(inPos, 1.0)).xyww;
}
