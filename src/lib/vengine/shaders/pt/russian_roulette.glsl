float rrsample = rand1D(rayPayloadPrimary);
if (rayPayloadPrimary.recursionDepth > RUSSIAN_ROULETTE_DEPTH)
{
    float maxBeta = max(rayPayloadPrimary.beta.x, max(rayPayloadPrimary.beta.y, rayPayloadPrimary.beta.z));
    if (rrsample >= maxBeta) {
        rayPayloadPrimary.stop = true;
        return;
    }

    rayPayloadPrimary.beta = rayPayloadPrimary.beta * (1.0 / maxBeta);
}