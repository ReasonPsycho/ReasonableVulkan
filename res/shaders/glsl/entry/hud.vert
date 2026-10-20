#version 450
#extension GL_ARB_shading_language_include : enable

#define HUD_GLSL
#define HUD_PC_GLSL

layout(push_constant) uniform HUDPushConstant {
    vec2 screenPosition;
    vec2 size;
    vec4 color;
    vec4 uvBounds;
    vec2 screenSize;
    uint textureIndex;
    uint hasTexture;
} push;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec4 outColor;
layout(location = 2) flat out uint outTextureIndex;
layout(location = 3) flat out uint outHasTexture;

void main()
{
    vec2 quadPositions[6] = vec2[](
        vec2(0.0, 0.0), // Top-Left
        vec2(0.0, 1.0), // Bottom-Left
        vec2(1.0, 0.0), // Top-Right
        vec2(1.0, 0.0), // Top-Right
        vec2(0.0, 1.0), // Bottom-Left
        vec2(1.0, 1.0)  // Bottom-Right
    );

    vec2 quadUVs[6] = vec2[](
        vec2(0.0, 0.0),
        vec2(0.0, 1.0),
        vec2(1.0, 0.0),
        vec2(1.0, 0.0),
        vec2(0.0, 1.0),
        vec2(1.0, 1.0)
    );

    vec2 localPos = quadPositions[gl_VertexIndex];
    vec2 pixelPos = push.screenPosition + localPos * push.size;

    // Convert pixel coordinates (0,0 is top-left) to Vulkan NDC (-1..1, -1..1)
    vec2 ndcPos = (pixelPos / push.screenSize) * 2.0 - 1.0;
    gl_Position = vec4(ndcPos, 0.0, 1.0);

    vec2 localUV = quadUVs[gl_VertexIndex];
    outUV = mix(push.uvBounds.xy, push.uvBounds.zw, localUV);
    outColor = push.color;
    outTextureIndex = push.textureIndex;
    outHasTexture = push.hasTexture;
}
