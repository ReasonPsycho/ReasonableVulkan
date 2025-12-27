struct PointLight {
    float3 position;
    float intensity;
    float3 color;
    float radius;
    float falloff;
    float padding[3];
};

[[vk::binding(2, 3)]]
StructuredBuffer<PointLight> pointLights;


float3 CalculatePointLight(PointLight light, float3 normal, float3 fragPos, float3 viewDir)
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

float3 AccumulatePointLights(
    float3 normal,
    float3 fragPos,
    float3 viewDir
) {
    float3 color = 0;

    for (int i = 0; i < lights.pointLightCount; i++) {
        CalculatePointLight(pointLights[i], normal, fragPos, viewDir);
        color += result;
    }

    return color;
}