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
#include "layoutDescriptors/SceneData.glsl"

void main()
{
    rayPayloadSecondary.stop = true;

    /* If secondary (shadow) ray missed, calculate volume transmittance if still inside a volume */
    rayPayloadSecondary.shadowed = false;

    if (rayPayloadSecondary.insideVolume)
    {
        uint volumeMaterialIndex = rayPayloadSecondary.volumeMaterialIndex;
        /* If a secondary ray missed while inside a volume, then use the z far of the camera for the end of the distance travelled inside the volume */
        float vtend = min(uint(sceneData.data.volumes.b), gl_RayTmaxEXT);
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
    } 
}