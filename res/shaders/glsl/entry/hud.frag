#version 450
#extension GL_ARB_shading_language_include : enable
#extension GL_EXT_nonuniform_qualifier : enable

#define HUD_GLSL
#define HUD_PC_GLSL

layout(binding = 0, set = 1) uniform sampler defaultSampler;
layout(binding = 1, set = 1) uniform texture2D globalTextures[];

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec4 inColor;
layout(location = 2) flat in uint inTextureIndex;
layout(location = 3) flat in uint inHasTexture;

layout(location = 0) out vec4 outFragColor;

void main()
{
    vec4 texColor = vec4(1.0);
    if (inHasTexture != 0u) {
        texColor = texture(sampler2D(globalTextures[nonuniformEXT(inTextureIndex)], defaultSampler), inUV);
    }
    vec4 finalColor = inColor * texColor;
    if (finalColor.a <= 0.001) {
        discard;
    }
    outFragColor = finalColor;
}
