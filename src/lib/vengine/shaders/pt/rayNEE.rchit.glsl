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
#include "../include/constants.glsl"
#include "../include/utils.glsl"
#include "../include/frame.glsl"
#include "structs_pt.glsl"

hitAttributeEXT vec2 attribs;

/* Ray payload */
layout(location = 2) rayPayloadInEXT RayPayloadNEE rayPayloadNEE;

/* Types for the arrays of vertices and indices of the currently hit object */
layout(buffer_reference, scalar) buffer Vertices {Vertex v[]; };
layout(buffer_reference, scalar) buffer Indices {ivec3  i[]; };

#include "layoutDescriptors/PathTracingData.glsl"
#include "layoutDescriptors/InstanceData.glsl"
#include "layoutDescriptors/MaterialData.glsl"
#include "layoutDescriptors/Textures.glsl"

void main()
{
    /* If the ray has no throughput, or we'be hit an emissive surface then exit early */
    if (rayPayloadNEE.stop)
        return;
    
    /* If this is called the NEE ray has hit a non emissive transparent surface with a volume change */

    #include "process_hit.glsl"
    #include "construct_frame.glsl"
    
    /* Account for volume transmittance, if inside a volume */
    if (rayPayloadNEE.insideVolume)
    {
        uint volumeMaterialIndex = rayPayloadNEE.volumeMaterialIndex;
        float vtend = gl_HitTEXT;
        float vtstart = rayPayloadNEE.vtmin;
        #include "process_volume_transmittance.glsl"

        rayPayloadNEE.throughput *= volumeTransmittance;

        /* If throughput is not large enough, then stop computation */
        if (max3(rayPayloadNEE.throughput) < EPSILON)
        {
            rayPayloadNEE.stop = true;
            rayPayloadNEE.throughput = vec3(0);
            rayPayloadNEE.emissive = vec3(0);
            return;
        } 
    }

    /* store parametric t of hit */
    rayPayloadNEE.vtmin = gl_HitTEXT;

    /* Register volume change */
    rayPayloadNEE.insideVolume = false;
    float newVolumeMaterialIndex;
    if (flipped)
        newVolumeMaterialIndex = instanceData.id.g;        
    else 
        newVolumeMaterialIndex = instanceData.id.b;
    if (newVolumeMaterialIndex != -1)
    {
        rayPayloadNEE.insideVolume = true;
        rayPayloadNEE.volumeMaterialIndex = uint(newVolumeMaterialIndex);
    }

    /* Set new ray origin for the NEE ray */
    rayPayloadNEE.origin = worldPosition;
}