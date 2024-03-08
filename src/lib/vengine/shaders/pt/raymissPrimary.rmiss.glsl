#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable

#include "defines_pt.glsl"

#include "../include/structs.glsl"
#include "structs_pt.glsl"

layout(location = 0) rayPayloadInEXT RayPayloadPrimary rayPayloadPrimary;

layout(set = 0, binding = 1) uniform readonly SceneDataUBO {
    SceneData data;
} sceneData;

layout(set = 5, binding = 0) uniform samplerCube skybox;

void main()
{
    rayPayloadPrimary.stop = true;

    vec3 rayDir = gl_WorldRayDirectionEXT;

    if (sceneData.data.background.a == 0.0)
    {
        /* solid color background */
        vec3 backgroundColor = sceneData.data.background.xyz;
    
        if (rayPayloadPrimary.depth == 0)
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

        if (rayPayloadPrimary.depth == 0)
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

        if (rayPayloadPrimary.depth == 0)
        {
            rayPayloadPrimary.albedo = solidColor;
            rayPayloadPrimary.normal = vec3(0, 0, 0);
            rayPayloadPrimary.radiance += solidColor * rayPayloadPrimary.beta;
        } else {
            rayPayloadPrimary.radiance += envMapColor * rayPayloadPrimary.beta;
        }
        
    }



}