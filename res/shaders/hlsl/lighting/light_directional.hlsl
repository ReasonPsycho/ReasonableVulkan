struct DirectionalLight {
    float3 direction;
    float intensity;
    float3 color;
    float padding;
};

[[vk::binding(1, 3)]] StructuredBuffer<DirectionalLight> directionalLightSSBO;

float3 CalculateDirectionalLight(DirectionalLight light, float3 normal, float3 fragPos, float3 viewDir)
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

float3 AccumulateDirectionalLights(
    float3 normal,
    float3 fragPos,
    float3 viewDir
) {
    float3 color = 0;

    for (int i = 0; i < lights.pointLightCount; i++) {
        CalculateDirectionalLight(pointLights[i], normal, fragPos, viewDir);
        color += result;
    }

    return color;
}