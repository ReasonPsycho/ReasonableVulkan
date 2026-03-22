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

// Bindings for Shadow Maps
layout(binding = 4, set = 3) uniform sampler2DArray shadowMaps;
layout(binding = 5, set = 3) uniform samplerCubeArray cubeShadowMaps;

const float SHININESS = 32.0;
const vec3 AMBIENT_LIGHT = vec3(0.2, 0.2, 0.2);

#endif
