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
#include "structs_pt.glsl"

hitAttributeEXT vec2 attribs;

/* Ray payload */
layout(location = 2) rayPayloadInEXT RayPayloadNEE rayPayloadNEE;

/* Types for the arrays of vertices and indices of the currently hit object */
layout(buffer_reference, scalar) buffer Vertices {Vertex v[]; };
layout(buffer_reference, scalar) buffer Indices {ivec3  i[]; };

layout(set = 0, binding = 2) uniform PathTracingData 
{
    uvec4 samplesBatchesDepthIndex;
    uvec4 lights;
} pathTracingData;

/* Descriptor with the buffer for the object description structs */
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

void main()
{
    /* Get the hit object geometry, and its material */
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

    /* Interpolate uvs */
    const vec2 uvs = v0.uv * barycentricCoords.x + v1.uv * barycentricCoords.y + v2.uv * barycentricCoords.z;
    vec2 tiledUV = uvs * material.uvTiling.rg;

    float alpha = material.albedo.a * texture(global_textures[nonuniformEXT(material.gTexturesIndices2.a)], tiledUV).r;
    float transparent = material.metallicRoughnessAO.a;
    vec3 emissive = material.emissive.a * material.emissive.rgb * texture(global_textures[nonuniformEXT(material.gTexturesIndices2.r)], tiledUV).rgb;

    /* If it's not an emissive surface, compute throughput and continue */
    if (isBlack(emissive, 0.05))
    {
        /* If not a transparent surface, stop NEE traversal */
        if (transparent < 1)
        {
            rayPayloadNEE.throughput = vec3(0);
            rayPayloadNEE.emissive = vec3(0);
            terminateRayEXT;
            return;
        }

        rayPayloadNEE.throughput *= vec3(1.0 - alpha);

        if (max3(rayPayloadNEE.throughput) > EPSILON)
        {
            /* If throughput is large enough continue */
            ignoreIntersectionEXT;
        } 
        else
        {
            /* Else stop NEE traversal */
            rayPayloadNEE.throughput = vec3(0);
            rayPayloadNEE.emissive = vec3(0);
            terminateRayEXT;
        }
        return;
    }

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
            rayPayloadNEE.throughput = vec3(0);
            rayPayloadNEE.emissive = vec3(0);
            terminateRayEXT;
        } 
    }

    /* Else it's an emissive surface, emissive surfaces shoudn't be transparent */

    /* Compute normal */
    const vec3 localNormal = v0.normal * barycentricCoords.x + v1.normal * barycentricCoords.y + v2.normal * barycentricCoords.z;
    vec3 worldNormal = normalize(vec3(localNormal * gl_WorldToObjectEXT));
    bool flipped = dot(worldNormal, gl_WorldRayDirectionEXT) > 0;

    /* Check if it's a back hit */
    if (flipped) {
        rayPayloadNEE.emissive = vec3(0);
        rayPayloadNEE.throughput = vec3(0);
        terminateRayEXT;
        return;
    }

    rayPayloadNEE.emissive = emissive;

    const vec3 localPosition = v0.position * barycentricCoords.x + v1.position * barycentricCoords.y + v2.position * barycentricCoords.z;
    const vec3 worldPosition = vec3(gl_ObjectToWorldEXT * vec4(localPosition, 1.0));

    /* Calculate pdf of sampling that point */
    vec3 v0WorldPos = vec3(gl_ObjectToWorldEXT * vec4(v0.position, 1.0));
    vec3 v1WorldPos = vec3(gl_ObjectToWorldEXT * vec4(v1.position, 1.0));
    vec3 v2WorldPos = vec3(gl_ObjectToWorldEXT * vec4(v2.position, 1.0));
    float triangleArea = 0.5 * length(cross(v1WorldPos - v0WorldPos, v2WorldPos - v0WorldPos));
    float sampledPointPdf = (1.0 / objResource.numTriangles) * ( 1 / triangleArea );
    const float dotProduct = dot(-gl_WorldRayDirectionEXT, worldNormal);
    if (dotProduct > 0)
    {
        float lightDirectPdf = sampledPointPdf * distanceSquared(gl_ObjectRayOriginEXT, worldPosition) / dotProduct;
        uint totalLights = pathTracingData.lights.r;
        rayPayloadNEE.pdf = lightDirectPdf * (1.0 / totalLights);
        terminateRayEXT;
    } else 
    {
        rayPayloadNEE.pdf = 0;
        rayPayloadNEE.emissive = vec3(0);
        terminateRayEXT;
    }
}