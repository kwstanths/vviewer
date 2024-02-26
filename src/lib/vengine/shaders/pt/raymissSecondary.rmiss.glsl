#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable

#include "defines_pt.glsl"

#include "../include/structs.glsl"
#include "structs_pt.glsl"

layout(location = 1) rayPayloadInEXT RayPayloadSecondary rayPayloadSecondary;

void main()
{
    rayPayloadSecondary.shadowed = false;
}