#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int16 : require
#extension GL_EXT_buffer_reference2 : require

#include "../include/structs.glsl"
#include "../include/frame.glsl"
#include "../include/sampling.glsl"
#include "../include/utils.glsl"
#include "../include/lighting.glsl"

hitAttributeEXT vec2 attribs;

/* Ray payload */
layout(location = 0) rayPayloadInEXT RayPayload rayPayload;
layout(location = 1) rayPayloadEXT bool shadowed;

/* Types for the arrays of vertices and indices of the currently hit object */
layout(buffer_reference, scalar) buffer Vertices {Vertex v[]; };
layout(buffer_reference, scalar) buffer Indices {ivec3  i[]; };

/* Descriptors */
layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;
layout(set = 0, binding = 2) uniform readonly SceneData {
    Scene data;
} scene;
layout(set = 0, binding = 3) uniform RayTracingData 
{
    uvec4 samplesBatchesDepthIndex;
    uvec4 lights;
} rayTracingData;

/* Descriptor with the buffer for the object description structs */
layout(set = 0, binding = 4, scalar) buffer ObjDesc_ 
{ 
	ObjDesc i[200]; 
} objDesc;

/* Descriptor with the buffer for the light structs */
layout(set = 0, binding = 5) uniform readonly Lights 
{ 
	Light i[20]; 
} lights;

/* Descriptor with materials */
layout(set = 1, binding = 0) uniform readonly Materials
{
	Material i[200];
} materials;

/* Descriptor for global textures arrays */
layout (set = 2, binding = 0) uniform sampler2D global_textures[];
layout (set = 2, binding = 0) uniform sampler3D global_textures_3d[];

#include "../include/rng.glsl"
#include "lightSampling.glsl"
#include "MIS.glsl"

void main()
{
	/* Get the hit object geometry, its vertices and its indices buffers */
	ObjDesc objResource = objDesc.i[gl_InstanceCustomIndexEXT];
	Indices indices = Indices(objResource.indexAddress);
	Vertices vertices = Vertices(objResource.vertexAddress);
	Material material = materials.i[objResource.materialIndex];

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
	/* Interpolate uvs */
	const vec2 uvs = v0.uv * barycentricCoords.x + v1.uv * barycentricCoords.y + v2.uv * barycentricCoords.z;

	/* Construct a geometry frame */
	Frame frame = Frame(worldNormal, worldTangent, worldBitangent);
	bool flipped = fixFrame(frame.normal, frame.tangent, frame.bitangent, gl_WorldRayDirectionEXT);

	vec2 tiledUV = uvs * material.uvTiling.rg;

	/* Apply normal map */
	vec3 newNormal = texture(global_textures[nonuniformEXT(material.gTexturesIndices2.g)], tiledUV).rgb;
	applyNormalToFrame(frame, processNormalFromNormalMap(newNormal));

	/* Material information */
	vec3 albedo = material.albedo.rgb * texture(global_textures[nonuniformEXT(material.gTexturesIndices1.r)], tiledUV).rgb;
    vec3 emissive = material.emissive.a * material.emissive.rgb * texture(global_textures[nonuniformEXT(material.gTexturesIndices2.r)], tiledUV).rgb;

	if (!isBlack(emissive) && !flipped) {
		if (rayPayload.depth == 0)
		{
			/* beta is 1 at depth 0 */
			rayPayload.radiance = emissive;
		} 
		else
		{
			/* If this mesh is emissive and the depth is not 1, weight the emissive contribution with MIS */
			vec3 v0WorldPos = vec3(gl_ObjectToWorldEXT * vec4(v0.position, 1.0));
			vec3 v1WorldPos = vec3(gl_ObjectToWorldEXT * vec4(v1.position, 1.0));
			vec3 v2WorldPos = vec3(gl_ObjectToWorldEXT * vec4(v2.position, 1.0));
			float triangleArea = 0.5 * length(cross(v1WorldPos - v0WorldPos, v2WorldPos - v0WorldPos));
			float sampledPointPdf = (1.0 / objResource.numTriangles) * ( 1 / triangleArea );

            const float dotProduct = dot(-gl_WorldRayDirectionEXT, worldNormal);
            float lightDirectPdf = sampledPointPdf * distanceSquared(gl_ObjectRayOriginEXT, worldPosition) / dotProduct;

			float weightMIS = PowerHeuristic(1, rayPayload.bsdfPdf, 1, lightDirectPdf);
			rayPayload.radiance += weightMIS * emissive * rayPayload.beta;
		}
		rayPayload.stop = true;
		return;
	}

	/* Light sampling */
	LightSamplingRecord lsr = sampleLight(worldPosition);
	if (!lsr.shadowed && !isBlack(lsr.radiance))
	{
		vec3 wi = worldToLocal(frame, lsr.direction);
		float cosTheta = clamp(wi.y, 0, 1);

		vec3 F = albedo * INV_PI * cosTheta;

		if (!isBlack(F))
		{
			if (lsr.isDeltaLight)
			{
				rayPayload.radiance += lsr.radiance * F * rayPayload.beta / lsr.pdf;
			}
			else 
			{
				float bsdfPdf = cosineSampleHemispherePdf(cosTheta);
				float weightMIS = PowerHeuristic(1, lsr.pdf, 1, bsdfPdf);
				
				rayPayload.radiance += weightMIS * lsr.radiance * F * rayPayload.beta / lsr.pdf;
			}
		}
	}

	/* BSDF sampling */
	float sampleDirectionPDF;
	vec3 sampleDirectionLocal = cosineSampleHemisphere(rand2D(rayPayload), sampleDirectionPDF);
	vec3 sampleDirectionWorld = localToWorld(frame, sampleDirectionLocal);
	
	rayPayload.origin = worldPosition;
	rayPayload.direction = sampleDirectionWorld;
	rayPayload.bsdfPdf = sampleDirectionPDF;

	/* float cosTheta = sampleDirectionLocal.y */
	rayPayload.beta *= albedo /* * INVPI * cosTheta / sampleDirectionPDF */;

}