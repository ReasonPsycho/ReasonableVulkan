#ifndef MATERIAL_SKYBOX_GLSL
#define MATERIAL_SKYBOX_GLSL

layout(binding = 0, set = 1) uniform sampler cubemapSampler;           // VK_DESCRIPTOR_TYPE_SAMPLER
layout(binding = 1, set = 1) uniform textureCube albedoTex;              // VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE

vec3 SampleAlbedo(vec3 uv)
{
    return texture(samplerCube(albedoTex, cubemapSampler), uv).rgb;
}

#endif
