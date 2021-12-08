struct PBRStandard {
    vec3 albedo;
    float metallic;
    float roughness;
};

#define PI 3.1415926538

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
} 

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

/**
    pbr: PBR info
    worldPos: shading position 
    normal: shading normal vector
    V: view vector
    L: light vector
    H: half vector
*/
vec3 calculatePBRStandardShading(PBRStandard pbr, vec3 worldPos, vec3 normal, vec3 V, vec3 L, vec3 H)
{
    vec3 F0 = vec3(0.04); 
    F0      = mix(F0, pbr.albedo, pbr.metallic);
    vec3 F  = fresnelSchlick(max(dot(H, V), 0.0), F0);
    
    float NDF = DistributionGGX(normal, H, pbr.roughness);       
    float G   = GeometrySmith(normal, V, L, pbr.roughness);  
    
    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, V), 0.0) * max(dot(normal, L), 0.0)  + 0.0001;
    vec3 specular     = numerator / denominator;  
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
  
    kD *= 1.0 - pbr.metallic;	
  
    float NdotL = max(dot(normal, L), 0.0);        
    return (kD * pbr.albedo / PI + specular) * NdotL;
}
