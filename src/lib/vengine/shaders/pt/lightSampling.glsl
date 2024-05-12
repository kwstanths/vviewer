LightSamplingRecord sampleLight(vec3 originPosition)
{
    LightSamplingRecord lsr;
    lsr.isDeltaLight = true;
    lsr.radiance = vec3(0, 0, 0);
    lsr.pdf = 1.0;

    uint totalLights = pathTracingData.lights.r;
    if (totalLights == 0)
    {
        return lsr;
    }

    /* Uniformly pick a light */
    float pdf = 1.0 / totalLights;
    uint randomLight = uint(rand1D(rayPayloadPrimary) * totalLights);
    LightInstance light = lightInstances.data[randomLight];

    float tmin = 0.001;
    float tmax = 10000.0;
    if (light.info.a == 0)
    {
        LightData ld = lightData.data[light.info.r];

        /* Point light */
        vec3 direction = light.position.rgb - originPosition;
        tmax = length(direction);
        lsr.direction = direction / tmax;
        lsr.radiance = ld.color.rgb * squareDistanceAttenuation(originPosition, light.position.rgb) * ld.color.a;
        lsr.pdf = pdf * 1.0;
        lsr.isDeltaLight = true;
    } 
    else if (light.info.a == 1)
    {
        LightData ld = lightData.data[light.info.r];

        /* Directional light */
        lsr.direction = -light.position.rgb;
        lsr.radiance = ld.color.rgb * ld.color.a;
        lsr.pdf = pdf * 1.0;
        lsr.isDeltaLight = true;
    }
    else if (light.info.a == 2)
    {
        /* Mesh light */
        uint meshIndex = uint(light.info.b);

        ObjDesc objResource = objDesc.i[meshIndex];
        Indices indices = Indices(objResource.indexAddress);
        Vertices vertices = Vertices(objResource.vertexAddress);
        MaterialData material = materialData.data[objResource.materialIndex];
        mat4 transform = mat4(light.position, light.position1, light.position2, vec4(0, 0, 0, 1));
        transform = transpose(transform);
        
        uint randomTriangle = uint(rand1D(rayPayloadPrimary) * objResource.numTriangles);
        ivec3 ind = indices.i[randomTriangle];
        float trianglePdf = 1.0 / objResource.numTriangles;
        
        /* Sample triangle */
        vec2 barycentricCoords = uniformSampleTriangle(rand2D(rayPayloadPrimary));
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
            vec3 emissive = material.emissive.a * material.emissive.rgb * texture(global_textures[nonuniformEXT(material.gTexturesIndices2.r)], sampledUV * material.uvTiling.rg).rgb;
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
        /* Trace shadow ray */
        rayPayloadSecondary.shadowed = true;
        rayPayloadSecondary.throughput = vec3(1.0);
        rayPayloadSecondary.vtmin = tmin;
        rayPayloadSecondary.insideVolume = rayPayloadPrimary.insideVolume;
        rayPayloadSecondary.volumeMaterialIndex = rayPayloadPrimary.volumeMaterialIndex;

        /* Slightly reduce tmax to make sure we don't hit the sampled surface at all */
        uint secondaryRayFlags = gl_RayFlagsSkipClosestHitShaderEXT;
        traceRayEXT(topLevelAS, secondaryRayFlags, 0xFF, 1, 3, 1, originPosition, tmin, lsr.direction, tmax - tmin, 1);		
        
        /* If shadowed is still true make sure throughput is zero */
        if (rayPayloadSecondary.shadowed) 
        {
            rayPayloadSecondary.throughput = vec3(0.0);
        }

        lsr.radiance *= rayPayloadSecondary.throughput;
    }
    
    return lsr;
}
