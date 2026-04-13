#extension GL_ARB_shading_language_include : enable

#ifndef LIGHTING_COMMON_GLSL
#define LIGHTING_COMMON_GLSL

struct LightInfo
{
    int directionalLightCount;
    int pointLightCount;
    int spotLightCount;
    float far_plane;
};

layout(binding = 0, set = 3) uniform lightInfo {
    LightInfo lights;
};

// Bindings for Shadow Maps (separate arrays per light type)
layout(binding = 4, set = 3) uniform texture2DArray directionalShadowMaps;
layout(binding = 5, set = 3) uniform textureCubeArray cubeShadowMaps; // point lights
layout(binding = 6, set = 3) uniform texture2DArray spotShadowMaps;
layout(binding = 7, set = 3) uniform sampler shadowSampler;

const float SHININESS = 32.0;
const vec3 AMBIENT_LIGHT = vec3(0.2, 0.2, 0.2);

float CalculateDirectionalShadow(vec4 fragPosLightSpace, int shadowMapIndex, float bias, float shadowStrength) {
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float currentDepth = projCoords.z;

    if (projCoords.z > 1.0 || projCoords.z < 0.0)
        return 0.0;
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(sampler2DArray(directionalShadowMaps, shadowSampler), 0).xy);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(sampler2DArray(directionalShadowMaps, shadowSampler), vec3(projCoords.xy + vec2(x, y) * texelSize, shadowMapIndex)).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    return shadow * shadowStrength;
}

float CalculateSpotShadow(vec4 fragPosLightSpace, int shadowMapIndex, float bias, float shadowStrength) {
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    float currentDepth = projCoords.z;

    if (projCoords.z > 1.0 || projCoords.z < 0.0)
        return 0.0;

    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(sampler2DArray(spotShadowMaps, shadowSampler), 0).xy);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(sampler2DArray(spotShadowMaps, shadowSampler), vec3(projCoords.xy + vec2(x, y) * texelSize, shadowMapIndex)).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return shadow * shadowStrength;
}

float CalculateCubeShadow(vec3 fragPos, vec3 lightPos, int shadowMapIndex, float bias, float shadowStrength) {
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight) / lights.far_plane;

    float shadow = 0.0;
    int samples = 20;
    vec3 sampleOffsetDirections[20] = vec3[]
    (
       vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
       vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
       vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
       vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
       vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
    );

    float viewDistance = length(fragPos - lightPos);
    float diskRadius = (1.0 + (viewDistance / lights.far_plane)) / 25.0;
    for(int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(samplerCubeArray(cubeShadowMaps, shadowSampler), vec4(fragToLight + sampleOffsetDirections[i] * diskRadius, shadowMapIndex)).r;
        if(currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);
    
    return shadow * shadowStrength;
}

#endif
