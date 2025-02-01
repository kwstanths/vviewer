{
    /* Volume information */
    MaterialData volumeMaterial = materialData.data[volumeMaterialIndex];
    vec3 sigma_a = volumeMaterial.albedo.rgb;
    vec3 sigma_s = volumeMaterial.metallicRoughnessAO.rgb;
    sigma_s = max(sigma_s, EPSILON);
    vec3 sigma_t = sigma_a + sigma_s;
    float g = max(min(volumeMaterial.emissive.r, 0.99), -0.99);

    vec3 worldRayDirection = normalize(gl_WorldRayDirectionEXT);
    vec3 wo = -worldRayDirection;

    /* Distance traveled inside volume */
    float distance_inside_volume = max(vtend - vtstart, EPSILON);
    
    /* Sample a random channel for scattering event */
    uint channel = min(uint(rand1D(rayPayloadPrimary) * 3U), 2U);
    float hit_distance = -log(1.0 - rand1D(rayPayloadPrimary)) / sigma_t[channel];

    /* If hit distance is smaller than the distance travelled inside the volume, then the ray will scatter */
    sampledMedium = hit_distance < distance_inside_volume;

    /* Calculate volume transmittance, either for full volume, or hit_distance */
    float transmittanceDistance = min(hit_distance, distance_inside_volume);
    vec3 transmittance = exp(-sigma_t * transmittanceDistance);
    
    /* Calculate beta */
    vec3 density = sampledMedium ? (sigma_t * transmittance) : transmittance;
    float pdf = 0;
    for (int i = 0; i < 3; ++i) {
        pdf += density[i];
    }
    pdf *= 0.3333333;
    if (pdf == 0) {
        pdf = 1.0;
    }

    /* Add transmittance up to current point */
    vec3 F = sampledMedium ? (transmittance * sigma_s / pdf) : (transmittance / pdf);
    rayPayloadPrimary.beta *= F;

    if (sampledMedium)
    {
        /* Calculate scattering event */
        vec3 scatteringPosition = rayPayloadPrimary.origin + (vtstart + hit_distance) * worldRayDirection;
        
        /* Light sampling */
        LightSamplingRecord lsr = sampleLight(scatteringPosition);
        if (!isBlack(lsr.radiance))
        {
            /* Calculate the propability of generating that scattering direction */
            float p = HG_p(wo, lsr.direction, g);
            vec3 F = vec3(p);

            if (!isBlack(F))
            {
                if (lsr.isDeltaLight)
                {
                    rayPayloadPrimary.radiance += lsr.radiance * F * rayPayloadPrimary.beta / lsr.pdf;
                }
                else 
                {
                    float scatteringPdf = p;
                    float weightMIS = PowerHeuristic(1, lsr.pdf, 1, scatteringPdf);
                
                    rayPayloadPrimary.radiance += weightMIS * lsr.radiance * F * rayPayloadPrimary.beta / lsr.pdf;
                }
            }
        }

        /* Calculate new scatter direction */
        vec3 sampleDirectionWorld;
        float sampleDirectionPDF = HG_Sample(wo, sampleDirectionWorld, rand2D(rayPayloadPrimary), g);
    
        rayPayloadPrimary.origin = scatteringPosition;
        rayPayloadPrimary.direction = sampleDirectionWorld;

        #include "next_event_estimation.glsl"
    }
}