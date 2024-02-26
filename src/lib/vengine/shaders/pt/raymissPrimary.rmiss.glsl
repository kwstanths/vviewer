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

    vec3 backgroundColor = textureLod(skybox, rayDir, 0).xyz;

    if (rayPayloadPrimary.depth == 0)
    {
        rayPayloadPrimary.albedo = sceneData.data.exposure.g * backgroundColor;
        rayPayloadPrimary.normal = vec3(0, 0, 0);
    }

    rayPayloadPrimary.radiance += sceneData.data.exposure.g * backgroundColor * rayPayloadPrimary.beta;
}