#version 450

#include "pbr.glsl"
#include "environmentMap.glsl"

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragWorldNormal;
layout(location = 2) in vec3 fragWorldTangent;
layout(location = 3) in vec3 fragWorldBiTangent;
layout(location = 4) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform CameraData {
    mat4 view;
    mat4 projection;
} cameraData;

layout(set = 2, binding = 0) uniform PBRMaterialData {
    vec4 albedo;
    vec4 metallicRoughnessAOEmissive;
} pbrMaterialData;
layout(set = 2, binding = 1) uniform sampler2D materialTextures[6];

layout(set = 3, binding = 1) uniform samplerCube skyboxIrradiance;

vec3 getCameraPosition(mat4 invViewMatrix)
{
    return vec3(invViewMatrix[3][0], invViewMatrix[3][1], invViewMatrix[3][2]);
}

void main() {

    vec3 lightColor = vec3(20, 20, 20);
    vec3 lightPosition = vec3(3, 3, 3);
    vec3 L = normalize(lightPosition - fragWorldPos);
    
    /* Normal mapping */
    mat3 TBN = mat3(fragWorldTangent, fragWorldBiTangent, fragWorldNormal);
    vec3 N = texture(materialTextures[5], fragUV).rgb;
    N = N * 2.0 - 1.0;
    N = normalize(TBN * N);
    
    vec3 cameraPosition = getCameraPosition(inverse(cameraData.view));
    vec3 V = normalize(cameraPosition - fragWorldPos);
    vec3 H = normalize(V + L);
    
    /* Calculate PBR components */
    PBRStandard pbr;
    pbr.albedo = pbrMaterialData.albedo.rgb * texture(materialTextures[0], fragUV).rgb;
    pbr.metallic = pbrMaterialData.metallicRoughnessAOEmissive.r * texture(materialTextures[1], fragUV).r;
    pbr.roughness = pbrMaterialData.metallicRoughnessAOEmissive.g * texture(materialTextures[2], fragUV).r;
    float ao = pbrMaterialData.metallicRoughnessAOEmissive.b * texture(materialTextures[3], fragUV).r;
    float emissive = pbrMaterialData.metallicRoughnessAOEmissive.a * texture(materialTextures[4], fragUV).r;
    
    /* Calculate attenuation */
    float distance    = length(lightPosition - fragWorldPos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance     = lightColor * attenuation; 
    
    /* Calculate ambient IBL */
    vec3 F0 = vec3(0.04); 
    F0      = mix(F0, pbr.albedo, pbr.metallic);
    vec3 kS = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, pbr.roughness); 
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - pbr.metallic;
    vec3 irradiance = texture(skyboxIrradiance, N).rgb;
    vec3 diffuse    = irradiance * pbr.albedo;
    vec3 ambient    = kD * diffuse * ao; 
    
    vec3 pbrShading = radiance * calculatePBRStandardShading(pbr, fragWorldPos, N, V, L, H);
    vec3 emission = pbr.albedo * emissive;
    outColor = vec4(ambient + pbrShading + emission, 1);
}