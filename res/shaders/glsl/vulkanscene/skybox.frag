#version 450

layout(location = 0) in vec3 inUVW;
layout(location = 0) out vec4 outFragColor;

void main()
{
    outFragColor = vec4(0.2, 0.3, 0.8, 1.0);
}
