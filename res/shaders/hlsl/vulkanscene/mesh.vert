// Copyright 2020 Google LLC

struct VSInput
{
    [[vk::location(0)]] float3 Pos : POSITION0;
    [[vk::location(1)]] float3 Normal : NORMAL0;
    [[vk::location(2)]] float2 TexCoord : TEXCOORD0;
    [[vk::location(3)]] float4 Color : COLOR0;
    [[vk::location(4)]] float3 Tangent : COLOR1;
    [[vk::location(5)]] float3 Bitangent : COLOR2;
};

struct UBO
{
    float4x4 projection;
    float4x4 view;
};

[[vk::binding(1, 0)]] StructuredBuffer<float4> directionalLightSSBO;
[[vk::binding(2, 0)]] StructuredBuffer<float4> pointLightSSBO;
[[vk::binding(3, 0)]] StructuredBuffer<float4> spotLightSSBO;

[[vk::binding(0, 0)]] cbuffer ubo { UBO ubo; }

struct PushConstants {
    float4x4 model;
};
[[vk::push_constant]] PushConstants pushConstants;

struct VSOutput
{
    float4 Pos : SV_POSITION;
    [[vk::location(0)]] float2 UV : TEXCOORD0;
    [[vk::location(1)]] float3 Normal : NORMAL0;
    [[vk::location(2)]] float3 Color : COLOR0;
    [[vk::location(3)]] float3 WorldPos : TEXCOORD1;
    [[vk::location(4)]] float3 Tangent : COLOR1;
    [[vk::location(5)]] float3 Bitangent : COLOR2;
};

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput)0;

    // Pass through texture coordinates
    output.UV = input.TexCoord;

    // Transform normal vector to world space using model matrix
    output.Normal = normalize(mul((float3x3)pushConstants.model, input.Normal));

    // Pass through tangent and bitangent (transformed to world space)
    output.Tangent = normalize(mul((float3x3)pushConstants.model, input.Tangent));
    output.Bitangent = normalize(mul((float3x3)pushConstants.model, input.Bitangent));

    // Pass through vertex color (convert from float4 to float3)
    output.Color = input.Color.rgb;

    // Calculate position in world space for lighting
    float4x4 modelView = mul(ubo.view, pushConstants.model);
    float4 pos = float4(input.Pos, 1.0);
    float4 worldPos = mul(modelView, pos);

    // Output clip space position
    output.Pos = mul(ubo.projection, worldPos);

    // Output world position for fragment shader
    output.WorldPos = worldPos.xyz;

    return output;
}