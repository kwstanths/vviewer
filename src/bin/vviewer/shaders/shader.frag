#version 450

#include "pbr.glsl"

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
    
    PBRStandard pbr;
    pbr.albedo = pbrMaterialData.albedo.rgb * texture(materialTextures[0], fragUV).rgb;
    pbr.metallic = pbrMaterialData.metallicRoughnessAOEmissive.r * texture(materialTextures[1], fragUV).r;
    pbr.roughness = pbrMaterialData.metallicRoughnessAOEmissive.g * texture(materialTextures[2], fragUV).r;
    float ao = pbrMaterialData.metallicRoughnessAOEmissive.b * texture(materialTextures[3], fragUV).r;
    float emissive = pbrMaterialData.metallicRoughnessAOEmissive.a * texture(materialTextures[4], fragUV).r;
    
    float distance    = length(lightPosition - fragWorldPos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance     = lightColor * attenuation; 
    
    vec3 ambient = vec3(0.05) * pbr.albedo * ao;
    vec3 pbrShading = radiance * calculatePBRStandardShading(pbr, fragWorldPos, N, V, L, H);
    vec3 emission = pbr.albedo * emissive;
    outColor = vec4(ambient + pbrShading + emission, 1);
}