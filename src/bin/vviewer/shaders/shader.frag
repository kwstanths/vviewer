#version 450

#include "pbr.glsl"

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform CameraData {
    mat4 view;
    mat4 projection;
} cameraData;

layout(set = 0, binding = 2) uniform PBRMaterialData {
    vec4 albedo;
    vec4 metallicRoughnessAOEmissive;
} pbrMaterialData;

layout(set = 0, binding = 3) uniform sampler2D albedoSampler[5];

vec3 getCameraPosition(mat4 invViewMatrix)
{
    return vec3(invViewMatrix[3][0], invViewMatrix[3][1], invViewMatrix[3][2]);
}

void main() {

    vec3 lightColor = vec3(20, 20, 20);
    vec3 lightPosition = vec3(3, 3, 3);
    vec3 L = normalize(lightPosition - fragWorldPos);
    
    vec3 cameraPosition = getCameraPosition(inverse(cameraData.view));
    vec3 V = normalize(cameraPosition - fragWorldPos);

    vec3 H = normalize(V + L);
    
    float distance    = length(lightPosition - fragWorldPos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance     = lightColor * attenuation; 
    
    PBRStandard pbr;
    pbr.albedo = pbrMaterialData.albedo.rgb * texture(albedoSampler[0], fragUV).rgb;
    pbr.metallic = pbrMaterialData.metallicRoughnessAOEmissive.r * texture(albedoSampler[1], fragUV).r;
    pbr.roughness = pbrMaterialData.metallicRoughnessAOEmissive.g * texture(albedoSampler[2], fragUV).r;
    float ao = pbrMaterialData.metallicRoughnessAOEmissive.b * texture(albedoSampler[3], fragUV).r;
    float emissive = pbrMaterialData.metallicRoughnessAOEmissive.a * texture(albedoSampler[4], fragUV).r;
    
    vec3 ambient = vec3(0.03) * pbr.albedo * ao;
    vec3 pbrShading = radiance * calculatePBRStandardShading(pbr, fragWorldPos, fragNormal, V, L, H);
    vec3 emission = pbr.albedo * emissive;
    outColor =  vec4(ambient + pbrShading + emission, 1);
}