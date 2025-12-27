#ifndef VERTEX_IO_GLSL
#define VERTEX_IO_GLSL

struct VSInput
{
    vec3 Pos;
    vec3 Normal;
    vec2 TexCoord;
    vec4 Color;
    vec3 Tangent;
    vec3 Bitangent;
};

struct VSOutput
{
    vec4 Pos;
    vec2 UV;
    vec3 Normal;
    vec3 Color;
    vec3 WorldPos;
    vec3 Tangent;
    vec3 Bitangent;
};

#endif
