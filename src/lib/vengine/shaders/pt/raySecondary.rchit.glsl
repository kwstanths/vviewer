#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int16 : require
#extension GL_EXT_buffer_reference2 : require

#include "defines_pt.glsl"

#include "../include/structs.glsl"
#include "../include/utils.glsl"
#include "../include/constants.glsl"
#include "../include/frame.glsl"
#include "structs_pt.glsl"

hitAttributeEXT vec2 attribs;

/* Ray payload */
layout(location = 1) rayPayloadInEXT RayPayloadSecondary rayPayloadSecondary;

/* Types for the arrays of vertices and indices of the currently hit object */
layout(buffer_reference, scalar) buffer Vertices {Vertex v[]; };
layout(buffer_reference, scalar) buffer Indices {ivec3  i[]; };

#include "layoutDescriptors/InstanceData.glsl"
#include "layoutDescriptors/MaterialData.glsl"
#include "layoutDescriptors/Textures.glsl"

void main()
{
    if (rayPayloadSecondary.stop)
        return;

    /* If this shader is called not stopped, then the secondary ray has hit a transparent surface with a volume change */

    #include "process_hit.glsl"
    #include "construct_frame.glsl"

    /* If the ray travelled inside a volume compute volume transmittance */
    if (rayPayloadSecondary.insideVolume)
    {
        uint volumeMaterialIndex = rayPayloadSecondary.volumeMaterialIndex;
        float vtend = gl_HitTEXT;
        float vtstart = rayPayloadSecondary.vtmin;
        #include "process_volume_transmittance.glsl"

        rayPayloadSecondary.throughput *= volumeTransmittance;
        /* If throughput is not large enough stop */
        if (max3(rayPayloadSecondary.throughput) < EPSILON)
        {
            rayPayloadSecondary.stop = true;
            rayPayloadSecondary.shadowed = true;
            return;
        } 
    }

    /* store parametric t of hit */
    rayPayloadSecondary.vtmin = gl_HitTEXT;

    /* Register volume change */
    rayPayloadSecondary.insideVolume = false;
    float newVolumeMaterialIndex;
    if (flipped)
        newVolumeMaterialIndex = instanceData.id.g;        
    else 
        newVolumeMaterialIndex = instanceData.id.b;
    if (newVolumeMaterialIndex != -1)
    {
        rayPayloadSecondary.insideVolume = true;
        rayPayloadSecondary.volumeMaterialIndex = uint(newVolumeMaterialIndex);
    }

    /* Set new ray origin for the secondary ray */
    rayPayloadSecondary.origin = worldPosition;
}