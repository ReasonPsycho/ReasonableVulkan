#version 450
#extension GL_ARB_shading_language_include : enable

#define UI_OVERLAY_GLSL
#define UI_PC_GLSL

layout(push_constant) uniform UIPushConstants {
    vec2 screenResolution;
    vec2 quadPosition;
    vec2 quadSize;
    vec2 pad0;
    vec4 uvRect;
    vec4 color;
    uint textureIndex;
} pc;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec4 outColor;
layout(location = 2) out flat uint outTextureIndex;

void main()
{
    vec2 positions[6] = vec2[](
        vec2(0.0, 0.0),
        vec2(1.0, 0.0),
        vec2(1.0, 1.0),
        vec2(0.0, 0.0),
        vec2(1.0, 1.0),
        vec2(0.0, 1.0)
    );

    vec2 unitPos = positions[gl_VertexIndex];
    vec2 pixelPos = pc.quadPosition + unitPos * pc.quadSize;
    // Map screen pixel space (0..res.x, 0..res.y) to NDC (-1..1, -1..1)
    vec2 ndc = (pixelPos / pc.screenResolution) * 2.0 - 1.0;
    gl_Position = vec4(ndc, 0.0, 1.0);

    outUV = mix(pc.uvRect.xy, pc.uvRect.zw, unitPos);
    outColor = pc.color;
    outTextureIndex = pc.textureIndex;
}
