#version 450

// PN patch data
struct PnPatch
{
    float b210;
    float b120;
    float b021;
    float b012;
    float b102;
    float b201;
    float b111;
    float n110;
    float n011;
    float n101;
};

// tessellation levels
layout (binding = 0) uniform UBO
{
    mat4 projection;
    mat4 model;
    float tessAlpha;
    float tessLevel;
} ubo;

layout(vertices=3) out;

layout(location = 0) in vec3 inNormal[];
layout(location = 1) in vec2 inUV[];

layout(location = 0) out vec3 outNormal[3];
layout(location = 3) out vec2 outUV[3];
layout(location = 6) out PnPatch outPatch[3];

float wij(int i, int j)
{
    return dot(gl_in[j].gl_Position.xyz - gl_in[i].gl_Position.xyz, inNormal[i]);
}

float vij(int i, int j)
{
    vec3 Pj_minus_Pi = gl_in[j].gl_Position.xyz
    - gl_in[i].gl_Position.xyz;
    vec3 Ni_plus_Nj  = inNormal[i]+inNormal[j];
    return 2.0*dot(Pj_minus_Pi, Ni_plus_Nj)/dot(Pj_minus_Pi, Pj_minus_Pi);
}

void main()
{
    // get data
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    outNormal[gl_InvocationID]            = inNormal[gl_InvocationID];
    outUV[gl_InvocationID]          = inUV[gl_InvocationID];

    // set base 
    float P0 = gl_in[0].gl_Position[gl_InvocationID];
    float P1 = gl_in[1].gl_Position[gl_InvocationID];
    float P2 = gl_in[2].gl_Position[gl_InvocationID];
    float N0 = inNormal[0][gl_InvocationID];
    float N1 = inNormal[1][gl_InvocationID];
    float N2 = inNormal[2][gl_InvocationID];

    // compute control points
    outPatch[gl_InvocationID].b210 = (2.0*P0 + P1 - wij(0, 1)*N0)/3.0;
    outPatch[gl_InvocationID].b120 = (2.0*P1 + P0 - wij(1, 0)*N1)/3.0;
    outPatch[gl_InvocationID].b021 = (2.0*P1 + P2 - wij(1, 2)*N1)/3.0;
    outPatch[gl_InvocationID].b012 = (2.0*P2 + P1 - wij(2, 1)*N2)/3.0;
    outPatch[gl_InvocationID].b102 = (2.0*P2 + P0 - wij(2, 0)*N2)/3.0;
    outPatch[gl_InvocationID].b201 = (2.0*P0 + P2 - wij(0, 2)*N0)/3.0;
    float E = (outPatch[gl_InvocationID].b210
    + outPatch[gl_InvocationID].b120
    + outPatch[gl_InvocationID].b021
    + outPatch[gl_InvocationID].b012
    + outPatch[gl_InvocationID].b102
    + outPatch[gl_InvocationID].b201) / 6.0;
    float V = (P0 + P1 + P2)/3.0;
    outPatch[gl_InvocationID].b111 = E + (E - V)*0.5;
    outPatch[gl_InvocationID].n110 = N0+N1-vij(0, 1)*(P1-P0);
    outPatch[gl_InvocationID].n011 = N1+N2-vij(1, 2)*(P2-P1);
    outPatch[gl_InvocationID].n101 = N2+N0-vij(2, 0)*(P0-P2);

    // set tess levels
    gl_TessLevelOuter[gl_InvocationID] = ubo.tessLevel;
    gl_TessLevelInner[0] = ubo.tessLevel;
}