vec3 calculateIBLContribution(PBRStandard pbr, vec3 N, vec3 V, samplerCube skyboxIrradiance, samplerCube skyboxPrefiltered){
    /* Calculate ambient IBL */
    vec3 F0 = vec3(0.04); 
    F0      = mix(F0, pbr.albedo, pbr.metallic);
    vec3 kS = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, pbr.roughness); 
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - pbr.metallic;
    vec3 irradiance = texture(skyboxIrradiance, N).rgb;
    vec3 diffuse    = irradiance * pbr.albedo;
    
    /* TODO better compute specular */
    vec3 R = reflect(-V, N);
    vec3 specular = textureLod(skyboxPrefiltered, R, pbr.roughness * 9.0).rgb;
    
    return (kD * diffuse + specular);
}