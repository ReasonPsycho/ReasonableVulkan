struct LightInfo
{
    int directionalLightCount;
    int pointLightCount;
    int spotLightCount;
    int padding;
};

[[vk::binding(0, 3)]]
cbuffer lightInfo { LightInfo lights; }

static const float SHININESS = 32.0;
static const float3 AMBIENT_LIGHT = float3(0.2, 0.2, 0.2);