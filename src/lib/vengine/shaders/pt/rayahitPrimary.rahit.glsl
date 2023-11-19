#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int16 : require
#extension GL_EXT_buffer_reference2 : require

#include "../include/structs.glsl"
#include "../include/constants.glsl"

hitAttributeEXT vec2 attribs;

/* Ray payload */
layout(location = 0) rayPayloadInEXT RayPayloadPrimary rayPayloadPrimary;

/* Types for the arrays of vertices and indices of the currently hit object */
layout(buffer_reference, scalar) buffer Vertices {Vertex v[]; };
layout(buffer_reference, scalar) buffer Indices {ivec3  i[]; };

/* Descriptor with the buffer for the object description structs */
layout(set = 0, binding = 4, scalar) buffer ObjDesc_ 
{ 
	ObjDesc i[200]; 
} objDesc;

/* Descriptor with materials */
layout(set = 1, binding = 0) uniform readonly MaterialDataUBO
{
	MaterialData data[200];
} materialData;

/* Descriptor for global textures arrays */
layout (set = 2, binding = 0) uniform sampler2D global_textures[];
layout (set = 2, binding = 0) uniform sampler3D global_textures_3d[];

#include "../include/rng.glsl"

void main()
{
	/* Get the hit object geometry, and its material */
	ObjDesc objResource = objDesc.i[gl_InstanceCustomIndexEXT];
	Indices indices = Indices(objResource.indexAddress);
	Vertices vertices = Vertices(objResource.vertexAddress);
	MaterialData material = materialData.data[objResource.materialIndex];

	/* Get hit triangle info */
	ivec3  ind = indices.i[gl_PrimitiveID];
	Vertex v0 = vertices.v[ind.x];
	Vertex v1 = vertices.v[ind.y];
	Vertex v2 = vertices.v[ind.z];

	/* Calculate bayrcentric coordiantes */
	const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

	/* Interpolate uvs */
	const vec2 uvs = v0.uv * barycentricCoords.x + v1.uv * barycentricCoords.y + v2.uv * barycentricCoords.z;
	vec2 tiledUV = uvs * material.uvTiling.rg;

    float alpha = material.albedo.a * texture(global_textures[nonuniformEXT(material.gTexturesIndices2.a)], tiledUV).r;

    float rand = rand1D(rayPayloadPrimary);
    if (alpha < EPSILON)
    {
        ignoreIntersectionEXT;
    } 
    else if(rand > alpha)
    {
        ignoreIntersectionEXT;
    }

}