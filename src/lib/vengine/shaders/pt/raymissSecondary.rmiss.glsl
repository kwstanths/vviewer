#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable

#include "defines_pt.glsl"

#include "../include/structs.glsl"
#include "../include/constants.glsl"
#include "../include/utils.glsl"
#include "structs_pt.glsl"

layout(location = 1) rayPayloadInEXT RayPayloadSecondary rayPayloadSecondary;

#include "layoutDescriptors/MaterialData.glsl"

void main()
{
    /* If secondary (shadow) ray missed, calculate volume transmittance if still inside a volume */

    if (rayPayloadSecondary.insideVolume)
    {
        uint volumeMaterialIndex = rayPayloadSecondary.volumeMaterialIndex;
        float vtend = gl_RayTmaxEXT;
        float vtstart = rayPayloadSecondary.vtmin;
        #include "process_volume_transmittance.glsl"

        rayPayloadSecondary.throughput *= volumeTransmittance;

        /* If throughput is large enough, then it's not shadowed */
        if (max3(rayPayloadSecondary.throughput) > EPSILON)
        {
            rayPayloadSecondary.shadowed = false;
        } 
        else
        {
            rayPayloadSecondary.shadowed = true;
        }

    } else {
        rayPayloadSecondary.shadowed = false;
    }

}