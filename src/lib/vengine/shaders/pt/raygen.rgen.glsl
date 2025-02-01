#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "defines_pt.glsl"

#include "../include/structs.glsl"
#include "structs_pt.glsl"

/* Main PT descriptor set */
layout(set = 0, binding = 0, rgba32f) uniform image2D outputImage[3];

#include "layoutDescriptors/SceneData.glsl"
#include "layoutDescriptors/PathTracingData.glsl"
#include "layoutDescriptors/TLAS.glsl"

#include "../include/sampling.glsl"
#include "../include/rng/rng.glsl"

layout(location = 0) rayPayloadEXT RayPayloadPrimary rayPayloadPrimary;

void main() 
{
    uint batchSize = pathTracingData.samplesBatchesDepthIndex.r;
    uint batches = pathTracingData.samplesBatchesDepthIndex.g;
    uint depth = pathTracingData.samplesBatchesDepthIndex.b;
    uint batchIndex = pathTracingData.samplesBatchesDepthIndex.a;
    uint totalSamples = batchSize * batches;

#ifdef SAMPLING_PMJ
    rayPayloadPrimary.samplesPerPixel = totalSamples;
#endif

#ifdef SAMPLING_RTGEMS
    rayPayloadPrimary.rngState = initRNG(gl_LaunchIDEXT.xy, gl_LaunchSizeEXT.xy, batchIndex);
#endif

    /* Calculate pixel uvs */
#ifdef SAMPLING_PMJ
    rayPayloadPrimary.pixel = gl_LaunchIDEXT.xy;
#endif
    const vec2 pixelLeftCorner = vec2(gl_LaunchIDEXT.xy);
    
    float lensRadius = sceneData.data.exposure.b;
    float focalDistance = sceneData.data.exposure.a;

    /* Check if starting camera is inside a volume */
    bool insideVolume = (sceneData.data.volumes.r != -1);

    vec3 cumRadiance = vec3(0);
    vec3 cumAlbedo = vec3(0);
    vec3 cumNormal = vec3(0);
    for(int s = 0; s < batchSize; s++) 
    {
#ifdef SAMPLING_PMJ
        /* Start sampling at random dimension to avoid clustering artifacts with low number of total samples */
        rayPayloadPrimary.dimension = (rayPayloadPrimary.pixel.y * gl_LaunchSizeEXT.x + rayPayloadPrimary.pixel.y);
        rayPayloadPrimary.sampleIndex = batchIndex * batchSize + s;
#endif

        /* Calculate first ray target offset */
        vec2 pixelOffset = rand2D(rayPayloadPrimary);
        const vec2 inUV = (pixelLeftCorner + pixelOffset) / vec2(gl_LaunchSizeEXT.xy);
        vec2 d = inUV * 2.0 - 1.0;
        vec4 target = sceneData.data.projectionInverse * vec4(d.x, d.y, 1, 1);

        /* Calculate new ray */
        vec4 originCameraSpace = vec4(0, 0, 0, 1);
        vec4 directionCameraSpace = vec4(normalize(target.xyz), 0);

        /* Depth of field */
        if (lensRadius > 0)
        {
            /* PBRT */
            vec2 lensOffset = rand2D(rayPayloadPrimary);
            vec2 lensOrigin = lensRadius * concentricSampleDisk(lensOffset);

            float ft = focalDistance / (-directionCameraSpace.z);   /* Camera forward is at -z */
            vec4 focusPoint = directionCameraSpace * ft;

            originCameraSpace = vec4(lensOrigin.x, lensOrigin.y, 0, 1);
            directionCameraSpace = vec4(normalize(focusPoint.xyz - originCameraSpace.xyz), 0);
        }

        vec4 origin = sceneData.data.viewInverse * originCameraSpace;
        vec4 direction = sceneData.data.viewInverse * directionCameraSpace;

        rayPayloadPrimary.origin = origin.xyz;
        rayPayloadPrimary.direction = direction.xyz;
        rayPayloadPrimary.surfaceDepth = 0;
        rayPayloadPrimary.insideVolume = insideVolume;
        rayPayloadPrimary.volumeMaterialIndex = int(sceneData.data.volumes.r);

        vec3 beta = vec3(1);
        vec3 radiance = vec3(0);
        vec3 albedo = vec3(0);
        vec3 normal = vec3(0);
        for (int d = 0; d < depth; d++)
        {
            /* Launch ray */
            rayPayloadPrimary.stop = false;
            rayPayloadPrimary.radiance = radiance;
            rayPayloadPrimary.beta = beta;
            rayPayloadPrimary.recursionDepth = d;
            rayPayloadPrimary.vtmin = 0.001;    /* When ray tracing starts over, volume entry parametric t is zero */

            uint primaryRayFlags = gl_RayFlagsNoneEXT;
            traceRayEXT(topLevelAS, primaryRayFlags, 0xff, 0, 3, 0, origin.xyz, 0.001, direction.xyz, 10000.0, 0);

            beta = rayPayloadPrimary.beta;
            radiance = rayPayloadPrimary.radiance;
            albedo = rayPayloadPrimary.albedo;
            normal = rayPayloadPrimary.normal;
            
            if (rayPayloadPrimary.stop) 
            {
                break;
            }

            origin.xyz = rayPayloadPrimary.origin;
            direction.xyz = rayPayloadPrimary.direction;
        }

        cumRadiance += radiance / totalSamples;
        cumAlbedo += albedo / totalSamples;
        cumNormal += normal / totalSamples;
    }
    
    /* Store results */
    ivec2 uv = ivec2(gl_LaunchIDEXT.xy);

    vec4 storedRadiance = imageLoad(outputImage[0], uv);
    vec4 newRadiance = vec4(storedRadiance.rgb + cumRadiance, 1.F);
    imageStore(outputImage[0], uv, newRadiance);

    vec4 storedAlbedo = imageLoad(outputImage[1], uv);
    vec4 newAlbedo = vec4(storedAlbedo.rgb + cumAlbedo, 1.F);
    imageStore(outputImage[1], uv, newAlbedo);

    vec4 storedNormal = imageLoad(outputImage[2], uv);
    vec4 newNormal = vec4(storedNormal.rgb + cumNormal, 1.F);
    imageStore(outputImage[2], uv, newNormal);
}