#define USE_POINT_LIGHTS 1
#define USE_DIR_LIGHTS   1

#include "common/frame_ubo.hlsl"
#include "common/vertex_io.hlsl"
#include "material/material_pbr.hlsl"
#include "lighting/lighting_common.hlsl"

#if USE_DIR_LIGHTS
#include "lighting/light_directional.hlsl"
#endif

#if USE_POINT_LIGHTS
#include "lighting/light_point.hlsl"
#endif

float4 main(VSOutput input) : SV_TARGET
{
    float3 albedo = SampleAlbedo(input.UV);
    float3 normal = normalize(input.Normal);
    float3 viewDir = normalize(frame.cameraPos - input.WorldPos);

    float3 color = AMBIENT_LIGHT * albedo;

#if USE_DIR_LIGHTS
    color += AccumulateDirectionalLights(normal, input.WorldPos, viewDir);
#endif

#if USE_POINT_LIGHTS
    color += AccumulatePointLights(normal, input.WorldPos, viewDir);
#endif

    color *= input.Color;

    return float4(saturate(color), 1.0);
}
