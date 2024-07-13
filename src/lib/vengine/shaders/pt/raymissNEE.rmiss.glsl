#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable

#include "defines_pt.glsl"

#include "../include/structs.glsl"
#include "structs_pt.glsl"

layout(location = 2) rayPayloadInEXT RayPayloadNEE rayPayloadNEE;

void main()
{
    // Account for sampling the environment map + volume transmittance
    // rayPayloadNEE.emissive = vec3(0, 0, 0);
    // rayPayloadNEE.pdf = 0;
}