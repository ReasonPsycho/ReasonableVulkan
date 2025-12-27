#extension GL_ARB_shading_language_include : enable

#ifndef LIGHTING_COMMON_GLSL
#define LIGHTING_COMMON_GLSL

struct LightInfo
{
    int directionalLightCount;
    int pointLightCount;
    int spotLightCount;
    int padding;
};

layout(binding = 0, set = 3) uniform lightInfo {
    LightInfo lights;
};

const float SHININESS = 32.0;
const vec3 AMBIENT_LIGHT = vec3(0.2, 0.2, 0.2);

#endif
