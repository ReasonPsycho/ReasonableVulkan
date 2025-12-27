struct VSInput
{
    [[vk::location(0)]] float3 Pos : POSITION0;
    [[vk::location(1)]] float3 Normal : NORMAL0;
    [[vk::location(2)]] float2 TexCoord : TEXCOORD0;
    [[vk::location(3)]] float4 Color : COLOR0;
    [[vk::location(4)]] float3 Tangent : COLOR1;
    [[vk::location(5)]] float3 Bitangent : COLOR2;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    [[vk::location(0)]] float2 UV;
    [[vk::location(1)]] float3 Normal;
    [[vk::location(2)]] float3 Color;
    [[vk::location(3)]] float3 WorldPos;
    [[vk::location(4)]] float3 Tangent;
    [[vk::location(5)]] float3 Bitangent;
};
