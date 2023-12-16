#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable

#include "../include/structs.glsl"

layout(location = 2) rayPayloadInEXT RayPayloadNEE rayPayloadNEE;

void main()
{
    // TODO environment map sampling pdf
    // rayPayloadNEE.emissive = vec3(0, 0, 0);
    // rayPayloadNEE.pdf = 0;
}