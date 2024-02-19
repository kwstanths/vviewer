#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int16 : require
#extension GL_EXT_buffer_reference2 : require

#include "../include/structs.glsl"
#include "../include/frame.glsl"
#include "../include/utils.glsl"
#include "../include/lighting.glsl"
#include "../include/brdfs/pbrStandard.glsl"

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

#include "../include/rng.glsl"
#include "lightSampling.glsl"
#include "MIS.glsl"

void main()
{
    /* Get the hit object geometry, its vertices and its indices buffers */
    ObjDesc objResource = objDesc.i[gl_InstanceCustomIndexEXT];
    Indices indices = Indices(objResource.indexAddress);
    Vertices vertices = Vertices(objResource.vertexAddress);
    MaterialData material = materialData.data[objResource.materialIndex];

    /* Get hit triangle info */
    ivec3  ind = indices.i[gl_PrimitiveID];
    Vertex v0 = vertices.v[ind.x];
    Vertex v1 = vertices.v[ind.y];
    Vertex v2 = vertices.v[ind.z];

    /* Calculate bayrcentric coordiantes */
    const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    /* Compute the coordinates of the hit position */
    const vec3 localPosition      = v0.position * barycentricCoords.x + v1.position * barycentricCoords.y + v2.position * barycentricCoords.z;
    const vec3 worldPosition = vec3(gl_ObjectToWorldEXT * vec4(localPosition, 1.0));
    /* Compute the normal, tangent and bitangent at hit position */
    const vec3 localNormal		= v0.normal * barycentricCoords.x + v1.normal * barycentricCoords.y + v2.normal * barycentricCoords.z;
    const vec3 localTangent 	= v0.tangent * barycentricCoords.x + v1.tangent * barycentricCoords.y + v2.tangent * barycentricCoords.z;
    const vec3 localBitangent	= v0.bitangent * barycentricCoords.x + v1.bitangent * barycentricCoords.y + v2.bitangent * barycentricCoords.z;
    vec3 worldNormal = normalize(vec3(localNormal * gl_WorldToObjectEXT));
    vec3 worldTangent = normalize(vec3(localTangent * gl_WorldToObjectEXT));
    vec3 worldBitangent = normalize(vec3(localBitangent * gl_WorldToObjectEXT));
    /* Interpolate uvs */
    const vec2 uvs = v0.uv * barycentricCoords.x + v1.uv * barycentricCoords.y + v2.uv * barycentricCoords.z;

    /* Construct a geometry frame */
    Frame frame = Frame(worldNormal, worldTangent, worldBitangent);
    bool flipped = fixFrame(frame.normal, frame.tangent, frame.bitangent, gl_WorldRayDirectionEXT);

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

    float ao = material.metallicRoughnessAO.b * texture(global_textures[nonuniformEXT(material.gTexturesIndices1.a)], tiledUV).r;
    vec3 emissive = material.emissive.a * material.emissive.rgb * texture(global_textures[nonuniformEXT(material.gTexturesIndices2.r)], tiledUV).rgb;

    /* Store first hit info */
    if (rayPayloadPrimary.depth == 0)
    {
        rayPayloadPrimary.albedo = pbr.albedo;
        rayPayloadPrimary.normal = frame.normal * 0.5 + vec3(0.5);
    }

    /* Add emissive of first hit */
    if (rayPayloadPrimary.depth == 0 && !isBlack(emissive, 0.1) && !flipped) {
        rayPayloadPrimary.radiance += emissive * rayPayloadPrimary.beta;
        rayPayloadPrimary.stop = true;
        return;
    }

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
                rayPayloadPrimary.radiance += lsr.radiance * F * rayPayloadPrimary.beta;
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
    
    rayPayloadPrimary.origin = worldPosition;
    rayPayloadPrimary.direction = sampleDirectionWorld;

    if (isBlack(F))
    {
        rayPayloadPrimary.stop = true;
        return;
    } 
    rayPayloadPrimary.beta *= clamp(F / sampleDirectionPDF, 0, 1);
    
    /* Next event estimation */
    rayPayloadNEE.throughput = 1.0;
    rayPayloadNEE.emissive = vec3(0);
    rayPayloadNEE.pdf = 0;
    uint neeRayFlags = gl_RayFlagsSkipClosestHitShaderEXT;
    traceRayEXT(topLevelAS, neeRayFlags, 0xFF, 2, 3, 2, worldPosition, 0.001, rayPayloadPrimary.direction, 10000.0, 2);

    if (!isBlack(rayPayloadNEE.emissive))
    {
        float weightMIS = PowerHeuristic(1, sampleDirectionPDF, 1, rayPayloadNEE.pdf);
        rayPayloadPrimary.radiance += weightMIS * rayPayloadNEE.throughput * rayPayloadNEE.emissive * rayPayloadPrimary.beta;
    }

}