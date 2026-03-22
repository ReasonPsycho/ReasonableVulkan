#ifndef LIGHTSPACEMATRIX_PC
#define LIGHTSPACEMATRIX_PC

layout(push_constant) uniform PushConstantsLSM {
    mat4 lightSpaceMatrix;
    vec3 lightPos;
} push_lsm;

#endif
