LightSamplingRecord sampleLight(vec3 originPosition)
{
	LightSamplingRecord lsr;
	lsr.shadowed = true;
	lsr.isDeltaLight = true;
	lsr.radiance = vec3(0, 0, 0);
	lsr.pdf = 1.0;

	uint totalLights = rayTracingData.lights.r;
	if (totalLights == 0)
	{
		return lsr;
	}

	/* Uniformly pick a light */
	float pdf = 1.0 / totalLights;
	uint randomLight = uint(rand1D(rayPayload) * totalLights);
	Light light = lights.i[randomLight];

	float tmin = 0.001;
	float tmax = 10000.0;
	if (light.position.a == 0)
	{
		/* Point light */
		vec3 direction = light.position.rgb - originPosition;
		tmax = length(direction);
		lsr.direction = direction / tmax;
		lsr.radiance = light.color.rgb * squareDistanceAttenuation(originPosition, light.position.rgb);
		lsr.pdf = pdf * 1.0;
		lsr.isDeltaLight = true;
	} 
	else if (light.position.a == 1)
	{
		/* Directional light */
		lsr.direction = -light.direction.rgb;
		lsr.radiance = light.color.rgb;
		lsr.pdf = pdf * 1.0;
		lsr.isDeltaLight = true;
	}
	else if (light.position.a == 2)
	{
		/* Mesh light */
		uint meshIndex = uint(light.direction.a);

		ObjDesc objResource = objDesc.i[meshIndex];
		Indices indices = Indices(objResource.indexAddress);
		Vertices vertices = Vertices(objResource.vertexAddress);
		Material material = materials.i[objResource.materialIndex];
		mat4 transform = mat4(vec4(light.position.xyz, 0), vec4(light.direction.xyz, 0), vec4(light.color.xyz, 0), vec4(light.transform.xyz, 1));
		
		uint randomTriangle = uint(rand1D(rayPayload) * objResource.numTriangles);
		ivec3 ind = indices.i[randomTriangle];
		float trianglePdf = 1.0 / objResource.numTriangles;
		
		/* Sample triangle */
		vec2 barycentricCoords = uniformSampleTriangle(rand2D(rayPayload));
		vec3 sampledBarycentricCoords = vec3(barycentricCoords, 1.F - barycentricCoords.x - barycentricCoords.y);

		/* Sampled triangle info */
		Vertex v0 = vertices.v[ind.x];
		Vertex v1 = vertices.v[ind.y];
		Vertex v2 = vertices.v[ind.z];

		vec3 v0PosTransformed = (transform * vec4(v0.position, 1)).xyz;
		vec3 v1PosTransformed = (transform * vec4(v1.position, 1)).xyz;
		vec3 v2PosTransformed = (transform * vec4(v2.position, 1)).xyz;

		float triangleArea = 0.5 * length(cross(v1PosTransformed - v0PosTransformed, v2PosTransformed - v0PosTransformed));
		float sampledPointPdf = 1.0 / triangleArea;

    	vec3 sampledPoint = sampledBarycentricCoords.x * v0.position + sampledBarycentricCoords.y * v1.position + sampledBarycentricCoords.z * v2.position;
    	vec3 sampledNormal = sampledBarycentricCoords.x * v0.normal + sampledBarycentricCoords.y * v1.normal + sampledBarycentricCoords.z * v2.normal;
		vec2 sampledUV = sampledBarycentricCoords.x * v0.uv + sampledBarycentricCoords.y * v1.uv + sampledBarycentricCoords.z * v2.uv;
		sampledPoint = (transform * vec4(sampledPoint, 1)).xyz;
		sampledNormal = (transpose(inverse(transform)) * vec4(sampledNormal, 0)).xyz;

		vec3 direction = sampledPoint - originPosition;
		tmax = length(direction);
		lsr.direction = direction / tmax;
		float dotProduct = dot(-lsr.direction, sampledNormal);
		if (dotProduct > 0)
		{
		    /* Get material information */
		    vec3 albedo = material.albedo.rgb * texture(global_textures[nonuniformEXT(material.gTexturesIndices1.r)], sampledUV * material.uvTiling.rg).rgb;
		    vec3 emissive = material.metallicRoughnessAOEmissive.a * texture(global_textures[nonuniformEXT(material.gTexturesIndices2.r)], sampledUV * material.uvTiling.rg).r * albedo;
		    lsr.radiance = emissive;
			
            lsr.pdf = pdf * trianglePdf * sampledPointPdf * distanceSquared(originPosition, sampledPoint) / dotProduct;
		} else {
			lsr.radiance = vec3(0, 0, 0);
			lsr.pdf = 0.0F;
		}

		lsr.isDeltaLight = false;
	}
	else if (light.position.a == 3)
	{
		/* TODO sample Environment map */

		lsr.isDeltaLight = false;
	}

    /* Shadow ray */
	if (!isBlack(lsr.radiance))
	{
		/* Trace shadow ray, set stb offset indices to match shadow hit/miss shader group indices */
		shadowed = true;

		/* Slightly reduce tmax to make sure we don't hit the sampled surface at all */
		traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT, 0xFF, 0, 0, 1, originPosition, tmin, lsr.direction, tmax - tmin, 1);		
		
		lsr.shadowed = shadowed;
	}
	
	return lsr;
}
