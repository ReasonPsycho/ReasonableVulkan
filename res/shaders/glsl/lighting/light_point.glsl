#extension GL_ARB_shading_language_include : enable

#ifndef LIGHT_POINT_GLSL
#define LIGHT_POINT_GLSL

#include "lighting_common.glsl"

struct PointLight {
    vec3 position;
    float intensity;
    vec3 color;
    float radius;
    float falloff;
    float shadowBias;
    bool castShadows;
    int shadowMapIndex;
    float shadowStrength;
    mat4 lightSpaceMatrices[6];
    float padding[2];
};

layout(binding = 2, set = 3) readonly buffer PointLightSSBO {
    PointLight pointLights[];
} pointLightSSBO;


vec3 CalculatePointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float distance = length(light.position - fragPos);

    // Attenuation with falloff
    float attenuation = 1.0 / (1.0 + light.falloff * distance * distance);
    attenuation *= smoothstep(light.radius, 0.0, distance); // Smooth cutoff at radius

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.color * diff * light.intensity * attenuation;

    // Specular
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), SHININESS);
    vec3 specular = light.color * spec * light.intensity * attenuation * 0.5;

    float shadow = 0.0;
    if (light.castShadows) {
        shadow = CalculateCubeShadow(fragPos, light.position, light.shadowMapIndex, light.shadowBias, light.shadowStrength);
    }

    return (1.0 - shadow) * (diffuse + specular);
}

vec3 AccumulatePointLights(
    vec3 normal,
    vec3 fragPos,
    vec3 viewDir
) {
    vec3 color = vec3(0.0);

    for (int i = 0; i < lights.pointLightCount; i++) {
        color += CalculatePointLight(pointLightSSBO.pointLights[i], normal, fragPos, viewDir);
    }

    return color;
}

#endif
