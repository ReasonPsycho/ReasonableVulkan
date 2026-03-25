#version 450
#extension GL_ARB_shading_language_include : enable

#include "../lighting/light_point.glsl"
#include "../common/light_model_pc.glsl"

layout (location = 0) in vec4 inFragPos;

void main()
{
    // get distance between fragment and light source
    float lightDistance = length(inFragPos.xyz - pointLightSSBO.pointLights[push_lm.lightIndex].position);
    
    // map to [0;1] range by dividing by far_plane
    lightDistance = lightDistance / lights.far_plane;
    
    // write this as modified depth
    gl_FragDepth = lightDistance;
}
