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

    /* Process volumetric hit */
    bool sampledMedium = false;
    if (rayPayloadPrimary.insideVolume)
    {
        uint volumeMaterialIndex = rayPayloadPrimary.volumeMaterialIndex;
        float vtstart = rayPayloadPrimary.vtmin;
        float vtend = gl_HitTEXT;
        #include "process_volume_hit.glsl"
    }

    /* If the volume wasn't sampled, process surface hit */
    if (!sampledMedium)
    {
        /* Tile uvs */
        vec2 tiledUV = uvs * material.uvTiling.rg;

        /* Apply normal map */
        vec3 newNormal = texture(global_textures[nonuniformEXT(material.gTexturesIndices2.g)], tiledUV).rgb;
        applyNormalToFrame(frame, processNormalFromNormalMap(newNormal));

        /* Material information */
        PBRStandard pbr;
        pbr.albedo = material.albedo.rgb * texture(global_textures[nonuniformEXT(material.gTexturesIndices1.r)], tiledUV).rgb;
        pbr.metallic = material.metallicRoughnessAO.r * texture(global_textures[nonuniformEXT(material.gTexturesIndices1.g)], tiledUV).r;
        pbr.roughness = material.metallicRoughnessAO.g * texture(global_textures[nonuniformEXT(material.gTexturesIndices1.b)], tiledUV).r;
        pbr.roughness = max(pbr.roughness, 0.035); /* Cap low rougness because of sampling problems */

        vec3 emissive = material.emissive.a * material.emissive.rgb * texture(global_textures[nonuniformEXT(material.gTexturesIndices2.r)], tiledUV).rgb;

        /* Store first hit info */
        if (rayPayloadPrimary.surfaceDepth == 0)
        {
            rayPayloadPrimary.albedo = pbr.albedo;
            rayPayloadPrimary.normal = frame.normal * 0.5 + vec3(0.5);
        }

        /* Add emissive of first hit surface */
        if (rayPayloadPrimary.surfaceDepth == 0 && !isBlack(emissive, 0.1) && !flipped) {
            rayPayloadPrimary.radiance += emissive * rayPayloadPrimary.beta;
            rayPayloadPrimary.stop = true;
            return;
        }

        rayPayloadPrimary.surfaceDepth += 1;

        vec3 wo = worldToLocal(frame, -gl_WorldRayDirectionEXT);

        /* Light sampling */
        LightSamplingRecord lsr = sampleLight(worldPosition);
        if (!isBlack(lsr.radiance))
        {
            vec3 wi = worldToLocal(frame, lsr.direction);

            vec3 F = evalPBRStandard(pbr, wi, wo, normalize(wo + wi));

            if (!isBlack(F))
            {
                if (lsr.isDeltaLight)
                {
                    rayPayloadPrimary.radiance += lsr.radiance * F * rayPayloadPrimary.beta / lsr.pdf;
                }
                else 
                {
                    float bsdfPdf = pdfPBRStandard(wi, wo, pbr);
                    float weightMIS = PowerHeuristic(1, lsr.pdf, 1, bsdfPdf);
                    rayPayloadPrimary.radiance += weightMIS * lsr.radiance * F * rayPayloadPrimary.beta / lsr.pdf;
                }
            }
        }

        /* BSDF sampling */
        float sampleDirectionPDF;
        vec3 sampleDirectionLocal;
        vec3 F = samplePBRStandard(sampleDirectionLocal, wo, sampleDirectionPDF, pbr, rand2D(rayPayloadPrimary), rand1D(rayPayloadPrimary));    
        vec3 sampleDirectionWorld = localToWorld(frame, sampleDirectionLocal);

        /* Set new direction */
        rayPayloadPrimary.origin = worldPosition;
        rayPayloadPrimary.direction = sampleDirectionWorld;

        /* Add throughput */
        if (isBlack(F))
        {
            rayPayloadPrimary.stop = true;
            return;
        } 
        rayPayloadPrimary.beta *= clamp(F / sampleDirectionPDF, 0, 1);

        #include "next_event_estimation.glsl"
    }

    #include "russian_roulette.glsl"
}