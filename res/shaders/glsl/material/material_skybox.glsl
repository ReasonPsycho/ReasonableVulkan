#ifndef MATERIAL_SKYBOX_GLSL
#define MATERIAL_SKYBOX_GLSL

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

layout(binding = 0, set = 1) uniform sampler cubemapSampler;           // VK_DESCRIPTOR_TYPE_SAMPLER
layout(binding = 2, set = 1) uniform textureCube globalCubeTextures[]; // Bindless cube textures
layout(std430, binding = 3, set = 1) readonly buffer MaterialBuffer {
    GPUMaterialData materials[];
};

vec3 SampleAlbedo(vec3 uv)
{
    GPUMaterialData mat = materials[push.materialIndex];
    return texture(samplerCube(globalCubeTextures[nonuniformEXT(mat.baseColorTexIndex)], cubemapSampler), uv).rgb;
}

#endif
