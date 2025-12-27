[[vk::binding(0, 1)]] SamplerState defaultSampler;
[[vk::binding(1, 1)]] Texture2D albedoTex;
[[vk::binding(2, 1)]] Texture2D normalTex;

float3 SampleAlbedo(float2 uv)
{
    return albedoTex.Sample(defaultSampler, uv).rgb;
}
