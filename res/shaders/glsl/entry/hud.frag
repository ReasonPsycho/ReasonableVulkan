#version 450
#extension GL_ARB_shading_language_include : enable
#extension GL_EXT_nonuniform_qualifier : enable

#define UI_OVERLAY_GLSL
#define MATERIAL_PBR_GLSL

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec4 inColor;
layout(location = 2) in flat uint inTextureIndex;

layout(set = 1, binding = 0) uniform sampler defaultSampler;
layout(set = 1, binding = 1) uniform texture2D textures[];

layout(location = 0) out vec4 outFragColor;

void main()
{
    vec4 texColor = vec4(1.0);
    if (inTextureIndex != 0xFFFFFFFF) {
        texColor = texture(sampler2D(textures[nonuniformEXT(inTextureIndex)], defaultSampler), inUV);
    }
    vec4 finalColor = texColor * inColor;
    if (finalColor.a <= 0.0001) {
        discard;
    }
    outFragColor = finalColor;
}
