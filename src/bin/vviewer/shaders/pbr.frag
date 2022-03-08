#version 450

#include "pbr.glsl"
#include "ibl.glsl"
#include "lighting.glsl"
#include "tonemapping.glsl"

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragWorldNormal;
layout(location = 2) in vec3 fragWorldTangent;
layout(location = 3) in vec3 fragWorldBiTangent;
layout(location = 4) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 projection;
    vec4 directionalLightDir;
    vec4 directionalLightColor;
    vec4 exposure;
} sceneData;

layout(set = 2, binding = 0) uniform PBRMaterialData {
    vec4 albedo;
    vec4 metallicRoughnessAOEmissive;
} pbrMaterialData;
layout(set = 2, binding = 1) uniform sampler2D materialTextures[7];

layout(set = 3, binding = 1) uniform samplerCube skyboxIrradiance;
layout(set = 3, binding = 2) uniform samplerCube skyboxPrefiltered;

vec3 getCameraPosition(mat4 invViewMatrix)
{
    return vec3(invViewMatrix[3][0], invViewMatrix[3][1], invViewMatrix[3][2]);
}

void main() {

    vec3 L = -sceneData.directionalLightDir.xyz;
    
    /* Normal mapping */
    mat3 TBN = mat3(fragWorldTangent, fragWorldBiTangent, fragWorldNormal);
    vec3 N = texture(materialTextures[5], fragUV).rgb;
    N = N * 2.0 - 1.0;
    N = normalize(TBN * N);
    
    vec3 cameraPosition = getCameraPosition(inverse(sceneData.view));
    vec3 V = normalize(cameraPosition - fragWorldPos);
    vec3 H = normalize(V + L);
    
    /* Calculate PBR components */
    PBRStandard pbr;
    pbr.albedo = pbrMaterialData.albedo.rgb * texture(materialTextures[0], fragUV).rgb;
    pbr.metallic = pbrMaterialData.metallicRoughnessAOEmissive.r * texture(materialTextures[1], fragUV).r;
    pbr.roughness = pbrMaterialData.metallicRoughnessAOEmissive.g * texture(materialTextures[2], fragUV).r;
    float ao = pbrMaterialData.metallicRoughnessAOEmissive.b * texture(materialTextures[3], fragUV).r;
    float emissive = pbrMaterialData.metallicRoughnessAOEmissive.a * texture(materialTextures[4], fragUV).r;
    
    
    vec3 ambient = ao * calculateIBLContribution(pbr, N, V, skyboxIrradiance, skyboxPrefiltered, materialTextures[6]);
    vec3 Lo = sceneData.directionalLightColor.xyz * calculatePBRStandardShading(pbr, fragWorldPos, N, V, L, H);
    vec3 emission = pbr.albedo * emissive;
    
    vec3 color = ambient + Lo + emission;
    
    color = tonemapDefault2(color, sceneData.exposure.r);
	
    outColor = vec4(color, 1);
}