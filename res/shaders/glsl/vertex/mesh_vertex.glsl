#extension GL_ARB_shading_language_include : enable

#ifndef MESH_VERTEX_GLSL
#define MESH_VERTEX_GLSL

#include "../common/scene_ubo.glsl"
#include "../common/vertex_io.glsl"

layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

VSOutput VertexTransform(VSInput vInput)
{
    VSOutput o;

    vec4 worldPos = push.model * vec4(vInput.Pos, 1.0);

    o.Pos = sceneUbo.projection * (sceneUbo.view * worldPos);
    o.WorldPos = worldPos.xyz;

    o.Normal = normalize(mat3(push.model) * vInput.Normal);
    o.Tangent = normalize(mat3(push.model) * vInput.Tangent);
    o.Bitangent = normalize(mat3(push.model) * vInput.Bitangent);

    o.UV = vInput.TexCoord;
    o.Color = vInput.Color.rgb;

    return o;
}

#endif
