#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable

#include "../include/structs.glsl"

layout(location = 1) rayPayloadInEXT RayPayloadSecondary rayPayloadSecondary;

void main()
{
	rayPayloadSecondary.shadowed = false;
}