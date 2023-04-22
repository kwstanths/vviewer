#include "constants.glsl"

struct PBRStandard {
    vec3 albedo;
    float metallic;
    float roughness;
};

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
} 

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(float NdotH, float roughness)
{
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH2 = NdotH * NdotH;
	
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

float GeometrySmithGGX(float NdotV, float NdotL, float roughness)
{
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

/**
    pbr: PBR info
    normal: shading normal vector
    V: view vector
    L: light vector
    H: half vector
    Vectors expressed in local Y-up space
*/
vec3 calculatePBRStandardShading(PBRStandard pbr, vec3 V, vec3 L, vec3 H)
{
    float NdotL = max(L.y, 0.0);
    float NdotV = max(V.y, 0.0);
    float NdotH = max(H.y, 0.0);

    vec3 F0 = vec3(0.04); 
    F0      = mix(F0, pbr.albedo, pbr.metallic);
    vec3 F  = fresnelSchlick(max(dot(H, V), 0.0), F0);
    
    float NDF = DistributionGGX(NdotH, pbr.roughness);
    float G   = GeometrySmithGGX(NdotV, NdotL, pbr.roughness);  
    
    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.0001;
    vec3 specular     = numerator / denominator;  
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
  
    kD *= 1.0 - pbr.metallic;	
  
    return (kD * pbr.albedo / PI + specular) * NdotL;
}
