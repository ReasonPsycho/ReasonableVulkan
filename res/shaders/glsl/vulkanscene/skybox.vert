#version 450

layout(binding = 0, set = 0) uniform UBO {
    mat4 projection;
    mat4 view;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

layout(location = 0) in vec3 inPos;
layout(location = 0) out vec3 outUVW;

void main()
{
    outUVW = inPos;
    
    mat4 viewMat = ubo.view;
    // HLSL matrix access [row][col]. In HLSL skybox.vert: viewMat[0][3] = 0.0 means first row, 4th column (translation x if row-major).
    // In GLSL column-major, viewMat[3][0] = 0.0 would be the same.
    // However, the intent is usually to strip translation.
    viewMat[3] = vec4(0.0, 0.0, 0.0, 1.0); 

    gl_Position = ubo.projection * viewMat * push.model * vec4(inPos, 1.0);
}
