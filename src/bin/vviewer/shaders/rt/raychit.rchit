#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int16 : require
#extension GL_EXT_buffer_reference2 : require

hitAttributeEXT vec2 attribs;

layout(location = 0) rayPayloadInEXT vec3 hitValue;
layout(location = 2) rayPayloadEXT bool shadowed;

/* Vertex data stored per vertex */
struct Vertex
{
    vec3 position;
    vec2 uv;
    vec3 normal;
    vec3 color;
    vec3 tangent;
    vec3 bitangent;
};
/* Array of vertices and indices of the hit object */
layout(buffer_reference, scalar) buffer Vertices {Vertex v[]; };
layout(buffer_reference, scalar) buffer Indices {u16vec3 i[]; };

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

/* Stores the address of the vertices and indices buffers of an object */
struct ObjDesc
{
	uint64_t vertexAddress;
	uint64_t indexAddress;
};
/* An array of objects in the scene */
layout(binding = 3, set = 0, scalar) buffer ObjDesc_ 
{ 
	ObjDesc i[]; 
} objDesc;

void main()
{
	/* Get the hit object, its vertices and its indices */
	ObjDesc objResource = objDesc.i[gl_InstanceCustomIndexEXT];
	Indices indices = Indices(objResource.indexAddress);
	Vertices vertices = Vertices(objResource.vertexAddress);

	/* Get triangle info */
	u16vec3 ind = indices.i[gl_PrimitiveID];
	Vertex v0 = vertices.v[ind.x];
	Vertex v1 = vertices.v[ind.y];
	Vertex v2 = vertices.v[ind.z];

	/* Calculate bayrcentric coordiantes */
	const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
	
	/* Compute the coordinates of the hit position */
	const vec3 position      = v0.position * barycentricCoords.x + v1.position * barycentricCoords.y + v2.position * barycentricCoords.z;
	const vec3 worldPos = vec3(gl_ObjectToWorldEXT * vec4(position, 1.0));  // Transforming the position to world space
	
	/* Compute the normal at hit position */
	const vec3 normal      = v0.normal * barycentricCoords.x + v1.normal * barycentricCoords.y + v2.normal * barycentricCoords.z;
	const vec3 worldNormal = normalize(vec3(normal * gl_WorldToObjectEXT));  // Transforming the normal to world space
	
	hitValue = clamp(vec3(dot(worldNormal, -sceneData.directionalLightDir.xyz)), 0, 1);
	
	/* Send shadow ray shadow ray */
	float tmin = 0.001;
	float tmax = 10000.0;
	vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
	shadowed = true;  
	// Trace shadow ray and offset indices to match shadow hit/miss shader group indices
	traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT, 0xFF, 1, 0, 1, origin, tmin, -sceneData.directionalLightDir.xyz, tmax, 2);
	if (shadowed) {
		hitValue *= 0.3;
	}
}