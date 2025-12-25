struct UBO
{
    float4x4 projection;
    float4x4 view;
    float4x4 viewProj;
    float3 cameraPos;
    float padding;
};

[[vk::binding(0, 0)]]  cbuffer ubo { UBO ubo; }

[[vk::binding(0, 1)]] SamplerState defaultSampler;
[[vk::binding(1, 1)]] Texture2D albedoTex;
[[vk::binding(2, 1)]] Texture2D normalTex;

struct DirectionalLight {
    float3 direction;
    float intensity;
    float3 color;
    float padding;
};

struct PointLight {
    float3 position;
    float intensity;
    float3 color;
    float radius;
    float falloff;
    float padding[3];
};

struct SpotLight {
    float3 position;
    float intensity;
    float3 direction;
    float innerAngle;
    float3 color;
    float outerAngle;
    float range;
    float padding[3];
};

[[vk::binding(1, 0)]] StructuredBuffer<DirectionalLight> directionalLightSSBO;
[[vk::binding(2, 0)]] StructuredBuffer<PointLight> pointLightSSBO;
[[vk::binding(3, 0)]] StructuredBuffer<SpotLight> spotLightSSBO;

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

// Constants
static const float SHININESS = 32.0;
static const float3 AMBIENT_LIGHT = float3(0.2, 0.2, 0.2);

float3 calculateDirectionalLight(DirectionalLight light, float3 normal, float3 fragPos, float3 viewDir)
{
    float3 lightDir = normalize(-light.direction);

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    float3 diffuse = light.color * diff * light.intensity;

    // Specular
    float3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), SHININESS);
    float3 specular = light.color * spec * light.intensity * 0.5;

    return diffuse + specular;
}

float3 calculatePointLight(PointLight light, float3 normal, float3 fragPos, float3 viewDir)
{
    float3 lightDir = normalize(light.position - fragPos);
    float distance = length(light.position - fragPos);

    // Attenuation with falloff
    float attenuation = 1.0 / (1.0 + light.falloff * distance * distance);
    attenuation *= smoothstep(light.radius, 0.0, distance); // Smooth cutoff at radius

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    float3 diffuse = light.color * diff * light.intensity * attenuation;

    // Specular
    float3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), SHININESS);
    float3 specular = light.color * spec * light.intensity * attenuation * 0.5;

    return diffuse + specular;
}

float3 calculateSpotLight(SpotLight light, float3 normal, float3 fragPos, float3 viewDir)
{
    float3 lightDir = normalize(light.position - fragPos);
    float distance = length(light.position - fragPos);

    // Check range
    if (distance > light.range)
        return float3(0.0, 0.0, 0.0);

    // Cone angle attenuation
    float theta = acos(dot(lightDir, normalize(-light.direction)));
    float innerRad = radians(light.innerAngle);
    float outerRad = radians(light.outerAngle);

    float spotIntensity = smoothstep(outerRad, innerRad, theta);

    // Distance attenuation
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
    attenuation *= spotIntensity;

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    float3 diffuse = light.color * diff * light.intensity * attenuation;

    // Specular
    float3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), SHININESS);
    float3 specular = light.color * spec * light.intensity * attenuation * 0.5;

    return diffuse + specular;
}

float4 main(VSOutput input) : SV_TARGET
{
    float4 texColor = albedoTex.Sample(defaultSampler, input.UV);
    float3 normal = normalize(input.Normal);
    float3 viewDir = normalize(ubo.cameraPos - input.WorldPos);

    // Start with ambient lighting
    float3 finalColor = AMBIENT_LIGHT * texColor.rgb;

    // Apply directional lights
    uint directionalLightCount;
    uint stride;
    directionalLightSSBO.GetDimensions(directionalLightCount, stride);

    for (uint i = 0; i < directionalLightCount; i++)
    {
        finalColor += calculateDirectionalLight(
            directionalLightSSBO[i],
            normal,
            input.WorldPos,
            viewDir
        ) * texColor.rgb;
    }

    // Apply point lights
    uint pointLightCount;
    pointLightSSBO.GetDimensions(pointLightCount, stride);

    for (uint i = 0; i < pointLightCount; i++)
    {
        finalColor += calculatePointLight(
            pointLightSSBO[i],
            normal,
            input.WorldPos,
            viewDir
        ) * texColor.rgb;
    }

    // Apply spot lights
    uint spotLightCount;
    spotLightSSBO.GetDimensions(spotLightCount, stride);

    for (uint i = 0; i < spotLightCount; i++)
    {
        finalColor += calculateSpotLight(
            spotLightSSBO[i],
            normal,
            input.WorldPos,
            viewDir
        ) * texColor.rgb;
    }

    // Apply vertex color tint
    finalColor *= input.Color;

    // Clamp to prevent overexposure
    finalColor = clamp(finalColor, 0.0, 1.0);

//finalColor = float3(1.0,1.0,1.0);
    return float4(finalColor, texColor.a);
}