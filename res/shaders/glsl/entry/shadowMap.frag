#version 450

layout (location = 0) in vec4 inFragPos;

#include "../common/lightSpaceMatrix_pc.glsl"
#include "../lighting/lighting_common.glsl"

void main()
{
   // get distance between fragment and light source
    float lightDistance = length(inFragPos.xyz - push_lsm.lightPos);

    // map to [0;1] range by dividing by far_plane
    lightDistance = lightDistance / lights.far_plane;

    // write this as modified depth
    gl_FragDepth = lightDistance;
}