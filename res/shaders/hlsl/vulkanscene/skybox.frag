// Copyright 2020 Google LLC

// Remove the sampler binding since we're not using it anymore
//[[vk::binding(1, 0)]] SamplerState samplerCubeMap : register(s1);
//[[vk::binding(1, 0)]] TextureCube textureCubeMap : register(t1);

float4 main([[vk::location(0)]] float3 inUVW : TEXCOORD0) : SV_TARGET
{
    // Return a constant blue color for the sky
    return float4(0.2, 0.3, 0.8, 1.0);
}