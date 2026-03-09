#ifndef MODEL_PC_GLSL
#define MODEL_PC_GLSL

layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

#endif
