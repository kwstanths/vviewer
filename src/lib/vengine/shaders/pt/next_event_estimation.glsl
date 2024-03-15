rayPayloadNEE.throughput = 1.0;
rayPayloadNEE.emissive = vec3(0);
rayPayloadNEE.pdf = 0;
uint neeRayFlags = gl_RayFlagsSkipClosestHitShaderEXT;
traceRayEXT(topLevelAS, neeRayFlags, 0xFF, 2, 3, 2, worldPosition, 0.001, rayPayloadPrimary.direction, 10000.0, 2);

if (!isBlack(rayPayloadNEE.emissive))
{
    float weightMIS = PowerHeuristic(1, sampleDirectionPDF, 1, rayPayloadNEE.pdf);
    rayPayloadPrimary.radiance += weightMIS * rayPayloadNEE.throughput * rayPayloadNEE.emissive * rayPayloadPrimary.beta;
}