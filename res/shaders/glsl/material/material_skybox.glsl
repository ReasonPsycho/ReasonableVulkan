#ifndef MATERIAL_SKYBOX_GLSL
#define MATERIAL_SKYBOX_GLSL

layout(binding = 0, set = 1) uniform sampler cubemapSampler;           // VK_DESCRIPTOR_TYPE_SAMPLER
layout(binding = 1, set = 1) uniform texture2D albedoTex;              // VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE

vec3 SampleAlbedo(vec2 uv)
{
    return texture(sampler2D(albedoTex, defaultSampler), uv).rgb;
}

#endif
