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
#include "structs_pt.glsl"

hitAttributeEXT vec2 attribs;

/* Ray payload */
layout(location = 0) rayPayloadInEXT RayPayloadPrimary rayPayloadPrimary;

/* Types for the arrays of vertices and indices of the currently hit object */
layout(buffer_reference, scalar) buffer Vertices {Vertex v[]; };
layout(buffer_reference, scalar) buffer Indices {ivec3  i[]; };

#include "layoutDescriptors/InstanceData.glsl"
#include "layoutDescriptors/MaterialData.glsl"
#include "layoutDescriptors/Textures.glsl"

#include "../include/rng/rng.glsl"

void main()
{
    // #include "process_hit.glsl"
    // #include "construct_frame.glsl"

    // vec2 tiledUV = uvs * material.uvTiling.rg;
    // float alpha = material.albedo.a * texture(global_textures[nonuniformEXT(material.gTexturesIndices2.a)], tiledUV).r;
    // float transparent = material.metallicRoughnessAO.a;

    // /* Check if material marked as transparent */
    // if (transparent > 0)
    // {
    //     /* If the ray traveled inside a volume */
    //     if (rayPayloadPrimary.insideVolume)
    //     {
    //         /* and this transparent surface constitutes a volume material change then accept the hit */
    //         uint volumeMaterialIndex = rayPayloadPrimary.volumeMaterialIndex;
    //         if (flipped && (instanceData.id.g != -1))
    //             terminateRayEXT;
    //         else if (flipped && (instanceData.id.b != -1))
    //             terminateRayEXT;
    //     } 
    //     else 
    //     {
            
    //     }

    //     float rand = rand1D(rayPayloadPrimary);
    //     if (alpha < EPSILON)
    //     {
    //         ignoreIntersectionEXT;
    //     } 
    //     else if(rand > alpha)
    //     {
    //         ignoreIntersectionEXT;
    //     }
    // }


}