struct SceneUBO
{
    float4x4 projection;
    float4x4 view;
    float4x4 viewProj;
    float3 cameraPos;
    float padding;
};

[[vk::binding(0, 0)]]
cbuffer ubo { SceneUBO sceneUbo; }