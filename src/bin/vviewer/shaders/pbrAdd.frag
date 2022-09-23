#version 450

#include "pbr.glsl"
#include "lighting.glsl"
#include "tonemapping.glsl"

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragWorldNormal;
layout(location = 2) in vec3 fragWorldTangent;
layout(location = 3) in vec3 fragWorldBiTangent;
layout(location = 4) in vec2 fragUV;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outHighlight;

layout(set = 0, binding = 0) uniform SceneData {
    mat4 view;
	mat4 viewInverse;
    mat4 projection;
	mat4 projectionInverse;
    vec4 directionalLightDir;
    vec4 directionalLightColor;
    vec4 exposure;
} sceneData;

layout(set = 2, binding = 0) uniform MaterialData {
    vec4 albedo;
    vec4 metallicRoughnessAOEmissive;
    vec4 uvTiling;
} materialData;
layout(set = 2, binding = 1) uniform sampler2D materialTextures[7];

layout(push_constant) uniform PushConsts {
	layout (offset = 0) vec4 lightPosition;
    layout (offset = 16) vec4 lightColor;
} pushConsts;

vec3 getCameraPosition(mat4 invViewMatrix)
{
    return vec3(invViewMatrix[3][0], invViewMatrix[3][1], invViewMatrix[3][2]);
}

void main() {
    vec3 lightWorldPos = pushConsts.lightPosition.rgb;
    vec3 lightColor = pushConsts.lightColor.rgb;
    vec2 tiledUV = materialData.uvTiling.rg * fragUV;
    
    vec3 L = normalize(lightWorldPos - fragWorldPos);
    
    /* Normal mapping */
    mat3 TBN = mat3(fragWorldTangent, fragWorldBiTangent, fragWorldNormal);
    vec3 N = texture(materialTextures[5], tiledUV).rgb;
    N = N * 2.0 - 1.0;
    N = normalize(TBN * N);
    
    vec3 cameraPosition = getCameraPosition(sceneData.viewInverse);
    vec3 V = normalize(cameraPosition - fragWorldPos);
    vec3 H = normalize(V + L);
    
    /* Calculate PBR components */
    PBRStandard pbr;
    pbr.albedo = materialData.albedo.rgb * texture(materialTextures[0], tiledUV).rgb;
    pbr.metallic = materialData.metallicRoughnessAOEmissive.r * texture(materialTextures[1], tiledUV).r;
    pbr.roughness = materialData.metallicRoughnessAOEmissive.g * texture(materialTextures[2], tiledUV).r;    
    vec3 Lo = lightColor * calculatePBRStandardShading(pbr, fragWorldPos, N, V, L, H) * squareDistanceAttenuation(fragWorldPos, lightWorldPos);
    
    vec3 color = Lo;
    
    color = tonemapDefault2(color, sceneData.exposure.r);
	
    outColor = vec4(color, 1);
	outHighlight = vec4(0, 0, 0, 0);
}
