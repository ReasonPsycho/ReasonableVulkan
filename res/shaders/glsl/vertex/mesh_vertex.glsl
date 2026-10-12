#extension GL_ARB_shading_language_include : enable

#ifndef MESH_VERTEX_GLSL
#define MESH_VERTEX_GLSL

#include "../common/scene_ubo.glsl"
#include "../common/vertex_io.glsl"
#include "../common/model_pc.glsl"

struct GPUInstanceData {
    mat4 modelMatrix;
    uint materialIndex;
    uint customData;
    uint padding0;
    uint padding1;
};

layout(std430, binding = 4, set = 1) readonly buffer InstanceBuffer {
    GPUInstanceData instances[];
};

VSOutput VertexTransform(VSInput vInput)
{
    VSOutput o;

    mat4 modelMatrix = instances[gl_InstanceIndex].modelMatrix;

    vec4 worldPos = modelMatrix * vec4(vInput.Pos, 1.0);

    o.Pos = sceneUbo.projection * (sceneUbo.view * worldPos);
    o.WorldPos = worldPos.xyz;

    o.Normal = normalize(mat3(modelMatrix) * vInput.Normal);
    o.Tangent = normalize(mat3(modelMatrix) * vInput.Tangent);
    o.Bitangent = normalize(mat3(modelMatrix) * vInput.Bitangent);

    o.UV = vInput.TexCoord;
    o.Color = vInput.Color.rgb;

    return o;
}

#endif
