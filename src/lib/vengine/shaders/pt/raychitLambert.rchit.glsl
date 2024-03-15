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
#include "../include/sampling.glsl"
#include "../include/utils.glsl"
#include "../include/lighting.glsl"
#include "structs_pt.glsl"

hitAttributeEXT vec2 attribs;

/* Ray payload */
layout(location = 0) rayPayloadInEXT RayPayloadPrimary rayPayloadPrimary;
layout(location = 1) rayPayloadEXT RayPayloadSecondary rayPayloadSecondary;
layout(location = 2) rayPayloadEXT RayPayloadNEE rayPayloadNEE;

/* Types for the arrays of vertices and indices of the currently hit object */
layout(buffer_reference, scalar) buffer Vertices {Vertex v[]; };
layout(buffer_reference, scalar) buffer Indices {ivec3  i[]; };

/* Main PT Descriptor set */
layout(set = 0, binding = 2) uniform PathTracingData 
{
    uvec4 samplesBatchesDepthIndex;
    uvec4 lights;
} pathTracingData;

/* Descriptor set with TLAS and buffer for object description */
layout(set = 1, binding = 0) uniform accelerationStructureEXT topLevelAS;
layout(set = 1, binding = 1, scalar) buffer ObjDesc_ 
{ 
    ObjDesc i[16384]; 
} objDesc;

/* Descriptor with materials */
layout(set = 2, binding = 0) uniform readonly MaterialDataUBO
{
    MaterialData data[512];
} materialData;

/* Descriptor for global textures arrays */
layout (set = 3, binding = 0) uniform sampler2D global_textures[];
layout (set = 3, binding = 0) uniform sampler3D global_textures_3d[];

/* Descriptor for lights and light instances */
layout(set = 4, binding = 0) uniform readonly LightDataUBO {
    LightData data[1024];
} lightData;

layout(set = 4, binding = 1) uniform readonly LightInstancesUBO {
    LightInstance data[1024];
} lightInstances;

#include "../include/rng/rng.glsl"

#include "lightSampling.glsl"
#include "MIS.glsl"

void main()
{
    #include "process_hit.glsl"

    /* Tile uvs */
    vec2 tiledUV = uvs * material.uvTiling.rg;

    /* Apply normal map */
    vec3 newNormal = texture(global_textures[nonuniformEXT(material.gTexturesIndices2.g)], tiledUV).rgb;
    applyNormalToFrame(frame, processNormalFromNormalMap(newNormal));

    /* Material information */
    vec3 albedo = material.albedo.rgb * texture(global_textures[nonuniformEXT(material.gTexturesIndices1.r)], tiledUV).rgb;
    vec3 emissive = material.emissive.a * material.emissive.rgb * texture(global_textures[nonuniformEXT(material.gTexturesIndices2.r)], tiledUV).rgb;

    /* Store first hit info */
    if (rayPayloadPrimary.depth == 0)
    {
        rayPayloadPrimary.albedo = albedo;
        rayPayloadPrimary.normal = frame.normal * 0.5 + vec3(0.5);
    }

    /* Add emissive of first hit */
    if (rayPayloadPrimary.depth == 0 && !isBlack(emissive, 0.05) && !flipped) {
        rayPayloadPrimary.radiance += emissive * rayPayloadPrimary.beta;
        rayPayloadPrimary.stop = true;
        return;
    }

    /* Light sampling */
    LightSamplingRecord lsr = sampleLight(worldPosition);
    if (!isBlack(lsr.radiance))
    {
        vec3 wi = worldToLocal(frame, lsr.direction);
        float cosTheta = clamp(wi.y, 0, 1);

        vec3 F = albedo * INV_PI * cosTheta;

        if (!isBlack(F))
        {
            if (lsr.isDeltaLight)
            {
                rayPayloadPrimary.radiance += lsr.radiance * F * rayPayloadPrimary.beta / lsr.pdf;
            }
            else 
            {
                float bsdfPdf = cosineSampleHemispherePdf(cosTheta);
                float weightMIS = PowerHeuristic(1, lsr.pdf, 1, bsdfPdf);
                
                rayPayloadPrimary.radiance += weightMIS * lsr.radiance * F * rayPayloadPrimary.beta / lsr.pdf;
            }
        }
    }

    /* BSDF sampling */
    float sampleDirectionPDF;
    vec3 sampleDirectionLocal = cosineSampleHemisphere(rand2D(rayPayloadPrimary), sampleDirectionPDF);
    vec3 sampleDirectionWorld = localToWorld(frame, sampleDirectionLocal);
    
    rayPayloadPrimary.origin = worldPosition;
    rayPayloadPrimary.direction = sampleDirectionWorld;

    /* Add throughput */
    /* float cosTheta = sampleDirectionLocal.y */
    rayPayloadPrimary.beta *= albedo /* * INV_PI * cosTheta / sampleDirectionPDF */;

    #include "next_event_estimation.glsl"

    #include "russian_roulette.glsl"
}