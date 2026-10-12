#ifndef MODEL_PC_GLSL
#define MODEL_PC_GLSL

layout(push_constant) uniform ModelPushConstant {
    mat4 model;
    uint materialIndex;
} push;

#endif
