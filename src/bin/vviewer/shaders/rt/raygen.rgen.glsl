#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "rng.glsl"
#include "structs.glsl"

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 1, set = 0, rgba8) uniform image2D image;
layout(binding = 2, set = 0) uniform SceneData 
{
    mat4 view;
	mat4 viewInverse;
    mat4 projection;
	mat4 projectionInverse;
    vec4 directionalLightDir;
    vec4 directionalLightColor;
    vec4 exposure;
} sceneData;
layout(binding = 3, set = 0) uniform RayTracingData 
{
    uvec4 samplesBatchesDepthIndex;
    uvec4 lights;
} rayTracingData;

layout(location = 0) rayPayloadEXT RayPayload rayPayload;

void main() 
{
	uint samples = rayTracingData.samplesBatchesDepthIndex.r;
	uint batches = rayTracingData.samplesBatchesDepthIndex.g;
	uint depth = rayTracingData.samplesBatchesDepthIndex.b;
	uint index = rayTracingData.samplesBatchesDepthIndex.a;
	uint totalSamples = samples * batches;

	/* Initialize a RNG */
	uint rngState = initRNG(gl_LaunchIDEXT.xy, gl_LaunchSizeEXT.xy, index);
	rayPayload.rngState = rngState;

	/* Calculate pixel uvs */
	const vec2 pixelLeftCorner = vec2(gl_LaunchIDEXT.xy);
	
	vec3 cumRadiance = vec3(0);
	for(int s = 0; s < samples; s++) 
	{
		/* Calculate first ray target offset */
		vec2 offset = vec2(rand(rngState), rand(rngState));
		const vec2 inUV = (pixelLeftCorner + offset) / vec2(gl_LaunchSizeEXT.xy);
		vec2 d = inUV * 2.0 - 1.0;

		/* Calculate first ray direction */
		vec4 origin = sceneData.viewInverse * vec4(0, 0, 0, 1);
		vec4 target = sceneData.projectionInverse * vec4(d.x, d.y, 1, 1) ;
		vec4 direction = sceneData.viewInverse * vec4(normalize(target.xyz), 0) ;

		vec3 beta = vec3(1);
		vec3 radiance = vec3(0);
		for (int d = 0; d < depth; d++)
		{
			/* Launch ray */
			rayPayload.stop = false;
			rayPayload.radiance = radiance;
			rayPayload.beta = beta;

			traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, origin.xyz, 0.001, direction.xyz, 10000.0, 0);

			beta = rayPayload.beta;
			radiance = rayPayload.radiance;
			
			if (rayPayload.stop) 
			{
				break;
			}

			origin.xyz = rayPayload.origin;
			direction.xyz = rayPayload.direction;
		}

		cumRadiance += radiance / totalSamples;
	}
	
	/* Store results */
	ivec2 uv = ivec2(gl_LaunchIDEXT.xy);
	vec4 storedRadiance = imageLoad(image, uv);

	vec4 newRadiance = storedRadiance + vec4(cumRadiance, 1.F);
	imageStore(image, uv, newRadiance);
}