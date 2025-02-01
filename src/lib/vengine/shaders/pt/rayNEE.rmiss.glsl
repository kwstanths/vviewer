#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable

#include "defines_pt.glsl"

#include "../include/structs.glsl"
#include "structs_pt.glsl"

layout(location = 2) rayPayloadInEXT RayPayloadNEE rayPayloadNEE;

void main()
{
    rayPayloadNEE.stop = true;
    // Account for sampling the environment map + volume the transmittance if we are still inside a volume
    // The environment map is not being sampled
    rayPayloadNEE.emissive = vec3(0, 0, 0);
    rayPayloadNEE.pdf = 0;
}