#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inColor;

layout (set = 0, binding = 0) uniform UBOCamera {
    mat4 projection;
    mat4 view;
} camera;

layout (set = 1, binding = 0) uniform UBOModel {
    mat4 matrix;
} model;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outUV;

out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    outNormal = inNormal;
    outColor = inColor;
    outUV = inUV;
    gl_Position = camera.projection * camera.view * model.matrix * vec4(inPos.xyz, 1.0);
}