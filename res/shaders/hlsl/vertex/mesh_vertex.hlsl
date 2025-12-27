struct PushConstants {
    float4x4 model;
};
[[vk::push_constant]] PushConstants push;

VSOutput VertexTransform(VSInput input)
{
    VSOutput o = (VSOutput)0;

    float4 worldPos = mul(push.model, float4(input.Pos, 1.0));

    o.Pos = mul(frame.projection, mul(frame.view, worldPos));
    o.WorldPos = worldPos.xyz;

    o.Normal = normalize(mul((float3x3)push.model, input.Normal));
    o.Tangent = normalize(mul((float3x3)push.model, input.Tangent));
    o.Bitangent = normalize(mul((float3x3)push.model, input.Bitangent));

    o.UV = input.TexCoord;
    o.Color = input.Color.rgb;

    return o;
}