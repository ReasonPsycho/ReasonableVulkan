#ifndef MATERIAL_PBR_GLSL
#define MATERIAL_PBR_GLSL

layout(binding = 0, set = 1) uniform sampler defaultSampler;           // VK_DESCRIPTOR_TYPE_SAMPLER
layout(binding = 1, set = 1) uniform texture2D albedoTex;              // VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
layout(binding = 2, set = 1) uniform texture2D normalTex;

vec3 SampleAlbedo(vec2 uv)
{
    return texture(sampler2D(albedoTex, defaultSampler), uv).rgb;
}

#endif
