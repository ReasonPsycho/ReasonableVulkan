#ifndef SCENE_UBO_GLSL
#define SCENE_UBO_GLSL

struct SceneUBO
{
    mat4 projection;
    mat4 view;
    mat4 viewProj;
    vec3 cameraPos;
    float padding;
};

layout(binding = 0, set = 0) uniform ubo {
    SceneUBO sceneUbo;
};

#endif
