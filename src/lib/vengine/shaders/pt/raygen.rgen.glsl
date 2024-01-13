#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "../include/structs.glsl"

layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;
layout(set = 0, binding = 1, rgba32f) uniform image2D outputImage[3];
layout(set = 0, binding = 2) uniform readonly SceneDataUBO {
    SceneData data;
} sceneData;
layout(set = 0, binding = 3) uniform PathTracingData 
{
    uvec4 samplesBatchesDepthIndex;
    uvec4 lights;
} pathTracingData;

#include "../include/sampling.glsl"
#include "../include/rng.glsl"

layout(location = 0) rayPayloadEXT RayPayloadPrimary rayPayloadPrimary;

void main() 
{
    uint batchSize = pathTracingData.samplesBatchesDepthIndex.r;
    uint batches = pathTracingData.samplesBatchesDepthIndex.g;
    uint depth = pathTracingData.samplesBatchesDepthIndex.b;
    uint batchIndex = pathTracingData.samplesBatchesDepthIndex.a;
    uint totalSamples = batchSize * batches;

    /* Initialize a RNG */
    uint rngState = initRNG(gl_LaunchIDEXT.xy, gl_LaunchSizeEXT.xy, batchIndex);
    rayPayloadPrimary.rngState = rngState;

    /* Calculate pixel uvs */
    const vec2 pixelLeftCorner = vec2(gl_LaunchIDEXT.xy);
    
    float lensRadius = sceneData.data.exposure.b;
    float focalDistance = sceneData.data.exposure.a;

    vec3 cumRadiance = vec3(0);
    vec3 cumAlbedo = vec3(0);
    vec3 cumNormal = vec3(0);
    for(int s = 0; s < batchSize; s++) 
    {
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
            rayPayloadPrimary.depth = d;

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