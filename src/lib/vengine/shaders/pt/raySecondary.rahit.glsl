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

    vec2 tiledUV = uvs * material.uvTiling.rg;

    float alpha = material.albedo.a * texture(global_textures[nonuniformEXT(material.gTexturesIndices2.a)], tiledUV).r;
    float transparent = material.metallicRoughnessAO.a;
    bool isTransparent = transparent >= 0.99;

    /* If not transparent make ray shadowed */
    if (!isTransparent)
    {
        rayPayloadSecondary.stop = true;
        rayPayloadSecondary.shadowed = true;
        terminateRayEXT;
    }

    /* surface is transparent */

    /* accumulate throughput of transparent surface */
    rayPayloadSecondary.throughput *= vec3(1.0 - alpha);

    /* If this surface constitutes a volume material change for a transparent surface, then accept the hit and call the closest hit shader */
    if (instanceData.id.g != instanceData.id.b)
    {
        terminateRayEXT;
    }

    /* If throughput is large enough continue */
    if (max3(rayPayloadSecondary.throughput) > EPSILON)
    {
        rayPayloadSecondary.shadowed = false;
        ignoreIntersectionEXT;
    } 
    else
    {
        rayPayloadSecondary.stop = true;
        rayPayloadSecondary.shadowed = true;
        terminateRayEXT;
    }
}