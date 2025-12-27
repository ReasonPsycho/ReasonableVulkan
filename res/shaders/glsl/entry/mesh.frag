#version 450
#extension GL_ARB_shading_language_include : enable

#define USE_POINT_LIGHTS 1
#define USE_DIR_LIGHTS   1

#include "../common/scene_ubo.glsl"
#include "../common/vertex_io.glsl"
#include "../material/material_pbr.glsl"
#include "../lighting/lighting_common.glsl"

#if USE_DIR_LIGHTS
#include "../lighting/light_directional.glsl"
#endif

#if USE_POINT_LIGHTS
#include "../lighting/light_point.glsl"
#endif

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec3 inWorldPos;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBitangent;

layout(location = 0) out vec4 outFragColor;

void main()
{
    vec3 albedo = SampleAlbedo(inUV);
    vec3 normal = normalize(inNormal);
    vec3 viewDir = normalize(sceneUbo.cameraPos - inWorldPos);

    vec3 color = AMBIENT_LIGHT * albedo;

    #if USE_DIR_LIGHTS
    color += AccumulateDirectionalLights(normal, inWorldPos, viewDir);
    #endif

    #if USE_POINT_LIGHTS
    color += AccumulatePointLights(normal, inWorldPos, viewDir);
    #endif

    color *= inColor;

    outFragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
