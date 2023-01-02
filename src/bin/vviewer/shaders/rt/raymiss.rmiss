#version 460
#extension GL_EXT_ray_tracing : require

#include "structs.glsl"
#include "rng.glsl"

layout(location = 0) rayPayloadInEXT RayPayload rayPayload;

void main()
{
    rayPayload.stop = true;
    
    vec3 rayDir = gl_WorldRayDirectionEXT;

    float t = 0.5 * (rayDir.y + 1.0);
    vec3 backgroundColor = (1.0 - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0);

	rayPayload.radiance += backgroundColor * rayPayload.beta;
}