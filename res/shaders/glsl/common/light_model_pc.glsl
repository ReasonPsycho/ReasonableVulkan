#ifndef LIGHT_MODEL_PC_GLSL
#define LIGHT_MODEL_PC_GLSL

layout(push_constant) uniform LightModelPushConstant {
    mat4 model;
    int lightIndex;
    int lightType; // 0: Directional, 1: Point, 2: Spot
} push_lm;

#endif
