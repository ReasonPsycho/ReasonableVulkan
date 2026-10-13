#ifndef INSTANCE_SSBO_GLSL
#define INSTANCE_SSBO_GLSL

struct GPUInstanceData {
    mat4 modelMatrix;
    uint materialIndex;
    uint customData;
    uint padding0;
    uint padding1;
};

layout(std430, binding = 4, set = 1) readonly buffer InstanceBuffer {
    GPUInstanceData instances[];
};

#endif
