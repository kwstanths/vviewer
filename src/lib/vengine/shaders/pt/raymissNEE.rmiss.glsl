#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable

#include "defines_pt.glsl"

#include "../include/structs.glsl"
#include "structs_pt.glsl"

layout(location = 2) rayPayloadInEXT RayPayloadNEE rayPayloadNEE;

/* Descriptor with materials */
layout(set = 2, binding = 0) uniform readonly MaterialDataUBO
{
    MaterialData data[512];
} materialData;

void main()
{
    // Account for sampling the environment map + volume transmittance
    // rayPayloadNEE.emissive = vec3(0, 0, 0);
    // rayPayloadNEE.pdf = 0;
}