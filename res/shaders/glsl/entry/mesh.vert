#version 450
#extension GL_ARB_shading_language_include : enable

#include "../common/scene_ubo.glsl"
#include "../common/vertex_io.glsl"
#include "../vertex/mesh_vertex.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inColor;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBitangent;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outColor;
layout(location = 3) out vec3 outWorldPos;
layout(location = 4) out vec3 outTangent;
layout(location = 5) out vec3 outBitangent;

void main()
{
    VSInput vInput;
    vInput.Pos = inPos;
    vInput.Normal = inNormal;
    vInput.TexCoord = inTexCoord;
    vInput.Color = inColor;
    vInput.Tangent = inTangent;
    vInput.Bitangent = inBitangent;

    VSOutput output_ = VertexTransform(vInput);

    gl_Position = output_.Pos;
    outUV = output_.UV;
    outNormal = output_.Normal;
    outColor = output_.Color;
    outWorldPos = output_.WorldPos;
    outTangent = output_.Tangent;
    outBitangent = output_.Bitangent;
}
