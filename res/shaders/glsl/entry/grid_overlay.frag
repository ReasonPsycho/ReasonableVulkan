#version 450
#extension GL_ARB_shading_language_include : enable

#define MODEL_PC_GLSL
#define TACTICAL_GRID_GLSL

#include "../common/scene_ubo.glsl"
#include "../common/vertex_io.glsl"

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec3 inWorldPos;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBitangent;

layout(location = 0) out vec4 outFragColor;

// Analytical screen-space anti-aliased grid lines using screen derivatives
float ComputeGridLine(vec2 coord, float lineWidth)
{
    vec2 grid = abs(fract(coord - 0.5) - 0.5);
    vec2 deriv = fwidth(coord);
    vec2 line = smoothstep(lineWidth * deriv, vec2(0.0), grid);
    return max(line.x, line.y);
}

void main()
{
    vec2 worldXZ = inWorldPos.xz;
    float cellSize = 1.0;
    vec2 cellCoord = worldXZ / cellSize;

    // Grid lines with smoothstep anti-aliasing
    float gridAlpha = ComputeGridLine(cellCoord, 1.2);

    // Procedural tactical holographic pulse & glow
    vec2 cellCenter = floor(cellCoord) + 0.5;
    vec2 localPos = abs(cellCoord - cellCenter) * 2.0; // 0 at center, 1 at edge
    float innerGlow = (1.0 - max(localPos.x, localPos.y)) * 0.35;

    // Outer boundary glow
    float edgeGlow = smoothstep(0.75, 0.98, max(localPos.x, localPos.y)) * 0.65;

    // Tactical base colors
    vec4 baseGridColor = vec4(0.15, 0.75, 0.95, 0.45);
    vec4 glowColor = vec4(0.2, 0.85, 1.0, 0.8);

    vec4 finalColor = baseGridColor * (gridAlpha * 0.85 + innerGlow);
    finalColor += glowColor * edgeGlow;

    // Soft decal border falloff
    vec2 decalBorder = smoothstep(vec2(0.0), vec2(0.05), inUV) * smoothstep(vec2(1.0), vec2(0.95), inUV);
    float decalAlpha = decalBorder.x * decalBorder.y;

    finalColor.a *= decalAlpha;

    if (finalColor.a <= 0.001) {
        discard;
    }

    outFragColor = finalColor;
}
