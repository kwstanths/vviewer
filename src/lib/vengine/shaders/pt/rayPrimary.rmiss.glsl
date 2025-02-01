#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_buffer_reference2 : require

#include "defines_pt.glsl"
#include "structs_pt.glsl"

#include "../include/structs.glsl"
#include "../include/constants.glsl"
#include "../include/frame.glsl"
#include "../include/rng/rng.glsl"
#include "../include/lighting.glsl"
#include "../include/utils.glsl"
#include "../include/sampling.glsl"
#include "../include/phaseFunctions.glsl"

layout(location = 0) rayPayloadInEXT RayPayloadPrimary rayPayloadPrimary;
layout(location = 1) rayPayloadEXT RayPayloadSecondary rayPayloadSecondary;
layout(location = 2) rayPayloadEXT RayPayloadNEE rayPayloadNEE;

/* Types for the arrays of vertices and indices of the currently hit object */
layout(buffer_reference, scalar) buffer Vertices {Vertex v[]; };
layout(buffer_reference, scalar) buffer Indices {ivec3  i[]; };

#include "layoutDescriptors/PathTracingData.glsl"
#include "layoutDescriptors/InstanceData.glsl"
#include "layoutDescriptors/TLAS.glsl"
#include "layoutDescriptors/SceneData.glsl"
#include "layoutDescriptors/Skybox.glsl"
#include "layoutDescriptors/MaterialData.glsl"
#include "layoutDescriptors/Lights.glsl"
#include "layoutDescriptors/Textures.glsl"

#include "lightSampling.glsl"
#include "MIS.glsl"

void main()
{
    /* If the ray is still inside a volume, then sample the volume */
    bool sampledMedium = false;
    if (rayPayloadPrimary.insideVolume)
    {
        uint volumeMaterialIndex = rayPayloadPrimary.volumeMaterialIndex;
        float vtstart = rayPayloadPrimary.vtmin;
        /* If a primary ray missed while inside a volume, then use the z far of the camera for the end of the distance travelled inside the volume */
        float vtend = min(uint(sceneData.data.volumes.b), gl_RayTmaxEXT);
        #include "process_volume_hit.glsl"
    }

    /* If the volume wasn't sampled, then terminate recursion */
    if (!sampledMedium)
    {
        rayPayloadPrimary.stop = true;

        vec3 rayDir = gl_WorldRayDirectionEXT;

        if (sceneData.data.background.a == 0.0)
        {
            /* solid color background */
            vec3 backgroundColor = sceneData.data.background.xyz;
        
            if (rayPayloadPrimary.surfaceDepth == 0)
            {
                rayPayloadPrimary.albedo = backgroundColor;
                rayPayloadPrimary.normal = vec3(0, 0, 0);
            }
        
            rayPayloadPrimary.radiance += backgroundColor * rayPayloadPrimary.beta;
        } 
        else if (sceneData.data.background.a == 1.0)
        {
            /* environment map background */
            vec3 backgroundColor = sceneData.data.exposure.g * textureLod(skybox, rayDir, 0).xyz;

            if (rayPayloadPrimary.surfaceDepth == 0)
            {
                rayPayloadPrimary.albedo = backgroundColor;
                rayPayloadPrimary.normal = vec3(0, 0, 0);
            }
            
            rayPayloadPrimary.radiance += backgroundColor * rayPayloadPrimary.beta;
        } 
        else if (sceneData.data.background.a == 2.0)
        {
            /* solid color background with environment map lighting */
            vec3 envMapColor = textureLod(skybox, rayDir, 0).xyz;
            vec3 solidColor = sceneData.data.background.xyz;

            if (rayPayloadPrimary.surfaceDepth == 0)
            {
                rayPayloadPrimary.albedo = solidColor;
                rayPayloadPrimary.normal = vec3(0, 0, 0);
                rayPayloadPrimary.radiance += solidColor * rayPayloadPrimary.beta;
            } else {
                rayPayloadPrimary.radiance += envMapColor * rayPayloadPrimary.beta;
            }
            
        }
        return;
    }

    #include "russian_roulette.glsl"
}