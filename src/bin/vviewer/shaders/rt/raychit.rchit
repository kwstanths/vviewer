#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int16 : require
#extension GL_EXT_buffer_reference2 : require

#include "structs.glsl"
#include "frame.glsl"
#include "sampling.glsl"

hitAttributeEXT vec2 attribs;

/* Ray payload */
layout(location = 0) rayPayloadInEXT RayPayload rayPayload;
layout(location = 1) rayPayloadEXT bool shadowed;

/* Types for the arrays of vertices and indices of the currently hit object */
layout(buffer_reference, scalar) buffer Vertices {Vertex v[]; };
layout(buffer_reference, scalar) buffer Indices {ivec3  i[]; };

/* Descriptors */
layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
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

/* Struct for the object description geometry of a mesh in the scene */
struct ObjDesc
{
    /* Pointers to GPU buffers */
	uint64_t vertexAddress;
	uint64_t indexAddress;
};
/* Descriptor with the buffer for the object description structs */
layout(binding = 4, set = 0, scalar) buffer ObjDesc_ 
{ 
	ObjDesc i[]; 
} objDesc;

void main()
{
	/* Get the hit object geometry, its vertices and its indices buffers */
	ObjDesc objResource = objDesc.i[gl_InstanceCustomIndexEXT];
	Indices indices = Indices(objResource.indexAddress);
	Vertices vertices = Vertices(objResource.vertexAddress);
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
	
	Frame frame = Frame(worldNormal, worldTangent, worldBitangent);
	bool flipped = fixFrame(frame.normal, frame.tangent, frame.bitangent, gl_WorldRayDirectionEXT);

	// /* light sampling test */
	// {
	// 	vec3 lightRadiance = sceneData.directionalLightColor.xyz * clamp(vec3(dot(worldNormal, -sceneData.directionalLightDir.xyz)), 0, 1) * rayPayload.beta;

	// 	/* Shadow ray */
	// 	float tmin = 0.001;
	// 	float tmax = 10000.0;
	// 	vec3 origin = worldPosition;
	// 	shadowed = true;
	// 	/* Trace shadow ray, set stb offset indices to match shadow hit/miss shader group indices */
	// 	traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT, 0xFF, 0, 0, 1, origin, tmin, -sceneData.directionalLightDir.xyz, tmax, 1);
		
	// 	if (!shadowed) {
	// 		rayPayload.radiance += lightRadiance;
	// 	}
	// }

	rayPayload.origin = worldPosition;

	float sampleDirectionPDF;
	vec3 sampleDirection = localToWorld(frame, cosineSampleHemisphere(rayPayload.rngState, sampleDirectionPDF));

	float cosTheta = abs(dot(worldNormal, sampleDirection));

	rayPayload.direction = sampleDirection;
	rayPayload.beta *= vec3(0.5, 0.5, 0.5) * cosTheta * INVPI / sampleDirectionPDF;
}