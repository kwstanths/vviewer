#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_ray_query : enable
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
#include "../include/lighting.glsl"
#include "../include/brdfs/pbrStandard.glsl"
#include "../include/phaseFunctions.glsl"
#include "structs_pt.glsl"

hitAttributeEXT vec2 attribs;

/* Ray payload */
layout(location = 0) rayPayloadInEXT RayPayloadPrimary rayPayloadPrimary;
layout(location = 1) rayPayloadEXT RayPayloadSecondary rayPayloadSecondary;
layout(location = 2) rayPayloadEXT RayPayloadNEE rayPayloadNEE;

/* Types for the arrays of vertices and indices of the currently hit object */
layout(buffer_reference, scalar) buffer Vertices {Vertex v[]; };
layout(buffer_reference, scalar) buffer Indices {ivec3  i[]; };

#include "layoutDescriptors/PathTracingData.glsl"
#include "layoutDescriptors/InstanceData.glsl"
#include "layoutDescriptors/TLAS.glsl"
#include "layoutDescriptors/MaterialData.glsl"
#include "layoutDescriptors/Textures.glsl"
#include "layoutDescriptors/Lights.glsl"

#include "../include/rng/rng.glsl"

#include "lightSampling.glsl"
#include "MIS.glsl"

void main()
{
    #include "process_hit.glsl"
    #include "construct_frame.glsl"

    if (flipped)
    {
        /* If it's a volume exit, process a volumetric hit */

        bool sampledMedium = false;
        uint volumeMaterialIndex = instanceData.materialIndex;
        float vtstart = rayPayloadPrimary.vtmin;
        float vtend = gl_HitTEXT;
        #include "process_volume_hit.glsl"

        if (!sampledMedium)
        {
            /* If the volume wasn't sampled, the ray will exit the volume */
            rayPayloadPrimary.insideVolume = false;
            rayPayloadPrimary.origin = worldPosition;
            rayPayloadPrimary.direction = normalize(gl_WorldRayDirectionEXT);

        }
        
        #include "russian_roulette.glsl"
    }
    else
    {
        rayPayloadPrimary.insideVolume = true;
        rayPayloadPrimary.vtmin = gl_HitTEXT;
        rayPayloadPrimary.volumeMaterialIndex = instanceData.materialIndex;
        rayPayloadPrimary.origin = worldPosition;
        rayPayloadPrimary.direction = normalize(gl_WorldRayDirectionEXT);
    }
    
    /* TODO first hit albedo and normal, Lambert material */
}