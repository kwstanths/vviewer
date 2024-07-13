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
#include "../include/frame.glsl"
#include "../include/utils.glsl"
#include "../include/constants.glsl"
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
    #include "process_hit.glsl"
    #include "construct_frame.glsl"

    if (flipped && rayPayloadSecondary.insideVolume)
    {
        /* 
            The ray should *always* be inside a volume here, but because the order in which the any-hit shaders are invoked is undefined,
            this prevents corner cases where the flipped part is executed before the unflipped
        */

        /* Exiting a volume boundary, calculate volume transmittance */

        rayPayloadSecondary.insideVolume = false;

        uint volumeMaterialIndex = instanceData.materialIndex;
        float vtend = gl_HitTEXT;
        float vtstart = rayPayloadSecondary.vtmin;
        #include "process_volume_transmittance.glsl"

        rayPayloadSecondary.throughput *= volumeTransmittance;

        /* If throughput is large enough continue */
        if (max3(rayPayloadSecondary.throughput) > EPSILON)
        {
            ignoreIntersectionEXT;
        } 
        else
        {
            rayPayloadSecondary.shadowed = true;
            terminateRayEXT;
        }

    } 
    else 
    {
        /* Store parametric t value at volume entry, the volume material and ignore intersection */

        rayPayloadSecondary.insideVolume = true;
        rayPayloadSecondary.vtmin = gl_HitTEXT;
        rayPayloadSecondary.volumeMaterialIndex = instanceData.materialIndex;
        ignoreIntersectionEXT;

    }
}