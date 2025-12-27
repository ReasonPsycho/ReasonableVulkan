#extension GL_ARB_shading_language_include : enable

#ifndef LIGHT_SPOT_GLSL
#define LIGHT_SPOT_GLSL

#include "lighting_common.glsl"

struct SpotLight {
    vec3 position;
    float intensity;
    vec3 direction;
    float innerAngle;
    vec3 color;
    float outerAngle;
    float range;
    float padding[3];
};

layout(binding = 3, set = 3) readonly buffer SpotLightSSBO {
    SpotLight spotLights[];
} spotLightSSBO;

vec3 CalculateSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float distance = length(light.position - fragPos);

    // Check range
    if (distance > light.range)
        return vec3(0.0, 0.0, 0.0);

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
    vec3 diffuse = light.color * diff * light.intensity * attenuation;

    // Specular
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), SHININESS);
    vec3 specular = light.color * spec * light.intensity * attenuation * 0.5;

    return diffuse + specular;
}

vec3 AccumulateSpotLights(
    vec3 normal,
    vec3 fragPos,
    vec3 viewDir
) {
    vec3 color = vec3(0.0);

    for (int i = 0; i < lights.spotLightCount; i++) {
        color += CalculateSpotLight(spotLightSSBO.spotLights[i], normal, fragPos, viewDir);
    }

    return color;
}

#endif
