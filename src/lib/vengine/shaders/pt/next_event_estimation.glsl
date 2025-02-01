{
    /* Trace next event estimation ray */
    float tmin = 0.0001;
    /* tmax will be used to calculate volume transmittance if the shadow ray misses, use the  camera zfar value for max */
    float tmax = sceneData.data.volumes.b;
    rayPayloadNEE.stop = false;
    rayPayloadNEE.origin = rayPayloadPrimary.origin;
    rayPayloadNEE.throughput = vec3(1.0);
    rayPayloadNEE.emissive = vec3(0);
    rayPayloadNEE.pdf = 0;
    rayPayloadNEE.insideVolume = rayPayloadPrimary.insideVolume;
    rayPayloadNEE.vtmin = 0.001;
    rayPayloadNEE.volumeMaterialIndex = rayPayloadPrimary.volumeMaterialIndex;

    uint neeRayFlags = gl_RayFlagsNoneEXT;

    uint depth = pathTracingData.samplesBatchesDepthIndex.b;
    for (int d = 0; d < depth; d++)
    {
        rayPayloadNEE.vtmin = tmin;
        traceRayEXT(topLevelAS, neeRayFlags, 0xFF, 2, 3, 2, rayPayloadNEE.origin, tmin, rayPayloadPrimary.direction, tmax, 2);

        if (rayPayloadNEE.stop)
            break;

    }
    
    if (!isBlack(rayPayloadNEE.emissive))
    {
        float weightMIS = PowerHeuristic(1, sampleDirectionPDF, 1, rayPayloadNEE.pdf);
        rayPayloadPrimary.radiance += weightMIS * rayPayloadNEE.throughput * rayPayloadNEE.emissive * rayPayloadPrimary.beta;
    }
}