#ifndef MATERIAL_PBR_GLSL
#define MATERIAL_PBR_GLSL

#extension GL_EXT_nonuniform_qualifier : enable
#include "../common/model_pc.glsl"

struct GPUMaterialData {
    vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    float alphaCutoff;
    uint baseColorTexIndex;
    uint normalTexIndex;
    uint metallicRoughnessTexIndex;
    uint emissiveTexIndex;
    uint occlusionTexIndex;
    uint flags;
    uint padding[3];
};

layout(binding = 0, set = 1) uniform sampler defaultSampler;           // VK_DESCRIPTOR_TYPE_SAMPLER
layout(binding = 1, set = 1) uniform texture2D globalTextures[];       // Bindless sampled textures
layout(std430, binding = 3, set = 1) readonly buffer MaterialBuffer {
    GPUMaterialData materials[];
};

vec3 SampleAlbedo(vec2 uv)
{
    GPUMaterialData mat = materials[push.materialIndex];
    return texture(sampler2D(globalTextures[nonuniformEXT(mat.baseColorTexIndex)], defaultSampler), uv).rgb * mat.baseColorFactor.rgb;
}

vec3 SampleNormal(vec2 uv)
{
    GPUMaterialData mat = materials[push.materialIndex];
    return texture(sampler2D(globalTextures[nonuniformEXT(mat.normalTexIndex)], defaultSampler), uv).rgb;
}

#endif
