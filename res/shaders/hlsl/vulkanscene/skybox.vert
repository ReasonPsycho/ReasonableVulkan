// Copyright 2020 Google LLC
// Copyright 2023 Sascha Willems

struct UBO
{
    float4x4 projection;
    float4x4 model;
    float4x4 normal;
    float4x4 view;
};

cbuffer ubo : register(b0) { UBO ubo; }

struct VSOutput
{
    float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 UVW : TEXCOORD0;
};

VSOutput main([[vk::location(0)]] float3 Pos : POSITION0)
{
VSOutput output = (VSOutput)0;

// Remove translation from view matrix
float4x4 viewMat = ubo.view;
viewMat[0][3] = 0.0;
viewMat[1][3] = 0.0;
viewMat[2][3] = 0.0;
output.UVW = Pos;
output.Pos = mul(ubo.projection, mul(viewMat, mul(ubo.model, float4(Pos.xyz, 1.0))));

return output;
}
