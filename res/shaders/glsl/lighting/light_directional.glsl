#extension GL_ARB_shading_language_include : enable

#ifndef LIGHT_DIRECTIONAL_GLSL
#define LIGHT_DIRECTIONAL_GLSL

#include "lighting_common.glsl"

struct DirectionalLight {
    vec3 direction;
    float intensity;
    vec3 color;
    float padding;
};

layout(binding = 1, set = 3) readonly buffer DirectionalLightSSBO {
    DirectionalLight directionalLights[];
} directionalLightSSBO;

vec3 CalculateDirectionalLight(DirectionalLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.color * diff * light.intensity;

    // Specular
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), SHININESS);
    vec3 specular = light.color * spec * light.intensity * 0.5;

    return diffuse + specular;
}

vec3 AccumulateDirectionalLights(
    vec3 normal,
    vec3 fragPos,
    vec3 viewDir
) {
    vec3 color = vec3(0.0);

    for (int i = 0; i < lights.directionalLightCount; i++) {
        color += CalculateDirectionalLight(directionalLightSSBO.directionalLights[i], normal, fragPos, viewDir);
    }

    return color;
}

#endif
