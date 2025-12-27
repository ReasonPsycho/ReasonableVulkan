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

[[vk::binding(3, 3)]] StructuredBuffer<SpotLight> spotLightSSBO;

float3 CalculateSpotLight(SpotLight light, float3 normal, float3 fragPos, float3 viewDir)
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

float3 AccumulateSpointLights(
    float3 normal,
    float3 fragPos,
    float3 viewDir
) {
    float3 color = 0;

    for (int i = 0; i < lights.pointLightCount; i++) {
        CalculateSpotLight(pointLights[i], normal, fragPos, viewDir);
        color += result;
    }

    return color;
}