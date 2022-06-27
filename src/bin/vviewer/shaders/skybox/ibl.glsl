vec3 prefilteredReflection(samplerCube skyboxPrefiltered, vec3 R, float roughness)
{
	const float MAX_REFLECTION_LOD = 9.0; // Number of mip levels on the input prefiltered map, default is 9.0, 512 resolution
	float lod = roughness * MAX_REFLECTION_LOD;
	float lodf = floor(lod);
	float lodc = ceil(lod);
	vec3 a = textureLod(skyboxPrefiltered, R, lodf).rgb;
	vec3 b = textureLod(skyboxPrefiltered, R, lodc).rgb;
	return mix(a, b, lod - lodf);
}

vec3 calculateIBLContribution(PBRStandard pbr, vec3 N, vec3 V, samplerCube skyboxIrradiance, samplerCube skyboxPrefiltered, sampler2D brdfLUT){
    /* Calculate ambient IBL */
    vec3 irradiance = texture(skyboxIrradiance, N).rgb;
    vec2 brdf = texture(brdfLUT, vec2(max(dot(N, V), 0.0), pbr.roughness)).rg;
    vec3 R = reflect(-V, N); 
	vec3 reflection = prefilteredReflection(skyboxPrefiltered, R, pbr.roughness).rgb;	
    
    vec3 F0 = vec3(0.04); 
    F0      = mix(F0, pbr.albedo, pbr.metallic);
    
    /* Diffuse ambient IBL */
    vec3 diffuse    = irradiance * pbr.albedo;
    
    vec3 kS = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, pbr.roughness); 
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - pbr.metallic;
    
    vec3 specular = reflection * (kS * brdf.x + brdf.y);
    
    return (kD * diffuse + specular);
}