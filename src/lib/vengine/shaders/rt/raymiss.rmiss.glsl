#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable

#include "../include/structs.glsl"
#include "../include/rng.glsl"

layout(location = 0) rayPayloadInEXT RayPayload rayPayload;

layout(set = 0, binding = 2) uniform readonly SceneData {
    Scene data;
} scene;

layout(set = 3, binding = 0) uniform samplerCube skybox;

void main()
{
    rayPayload.stop = true;
    
    vec3 rayDir = gl_WorldRayDirectionEXT;

    vec3 backgroundColor = textureLod(skybox, rayDir, 0).xyz;

	rayPayload.radiance += scene.data.exposure.g * backgroundColor * rayPayload.beta;
}