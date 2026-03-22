#ifndef CUBELIGHTSPACEMATRIX_PC
#define CUBELIGHTSPACEMATRIX_PC

layout(push_constant) uniform PushConstantsCLSM {
    mat4 lightSpaceMatrices[6];
    vec3 lightPos;
} push_clsm;

#endif
