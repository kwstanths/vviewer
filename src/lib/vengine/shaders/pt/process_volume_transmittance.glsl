/* Volume information */
MaterialData volumeMaterial = materialData.data[volumeMaterialIndex];
vec3 sigma_a = volumeMaterial.albedo.rgb;
vec3 sigma_s = volumeMaterial.metallicRoughnessAO.rgb;
sigma_s = max(sigma_s, EPSILON);
vec3 sigma_t = sigma_a + sigma_s;
float g = volumeMaterial.emissive.r;

float distance_inside_volume = max(vtend - vtstart, EPSILON);
vec3 volumeTransmittance = exp(-sigma_t * distance_inside_volume);