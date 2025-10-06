
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
    float4x4 model;
    float4x4 normal;
    float4x4 view;
    float4 lightpos;
};

[[vk::binding(0, 0)]]  cbuffer ubo { UBO ubo; }

struct VSOutput
{
    float4 Pos : SV_POSITION;
[[vk::location(0)]] float2 UV : TEXCOORD0;
[[vk::location(1)]] float3 Normal : NORMAL0;
[[vk::location(2)]] float3 Color : COLOR0;
[[vk::location(3)]] float3 EyePos : POSITION0;
[[vk::location(4)]] float3 LightVec : TEXCOORD2;
[[vk::location(5)]] float3 Tangent : COLOR1;
[[vk::location(6)]] float3 Bitangent : COLOR2;
};

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput)0;
    
    // Pass through texture coordinates
    output.UV = input.TexCoord;
    
    // Transform normal vector to world space
    output.Normal = normalize(mul((float3x3)ubo.normal, input.Normal));
    
    // Pass through tangent and bitangent (transformed to world space)
    output.Tangent = normalize(mul((float3x3)ubo.normal, input.Tangent));
    output.Bitangent = normalize(mul((float3x3)ubo.normal, input.Bitangent));
    
    // Pass through vertex color (convert from float4 to float3)
    output.Color = input.Color.rgb;
    
    // Calculate position in view space for lighting
    float4x4 modelView = mul(ubo.view, ubo.model);
    float4 pos = float4(input.Pos, 1.0);
    float4 worldPos = mul(modelView, pos);
    
    // Output clip space position
    output.Pos = mul(ubo.projection, worldPos);
    
    // Calculate eye position and light vector for lighting
    output.EyePos = worldPos.xyz;
    float4 lightPos = mul(modelView, float4(ubo.lightpos.xyz, 1.0));
    output.LightVec = normalize(lightPos.xyz - output.EyePos);
    
    return output;
}