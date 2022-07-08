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
} materialData;
layout(set = 2, binding = 1) uniform sampler2D materialTextures[4];

layout(set = 3, binding = 1) uniform samplerCube skyboxIrradiance;

layout(push_constant) uniform PushConsts {
	layout (offset = 0) vec4 selected;
} pushConsts;

vec3 getCameraPosition(mat4 invViewMatrix)
{
    return vec3(invViewMatrix[3][0], invViewMatrix[3][1], invViewMatrix[3][2]);
}

void main() {

	vec3 cameraPosition = getCameraPosition(inverse(sceneData.view));
    vec3 V = normalize(cameraPosition - fragWorldPos);

    vec3 L = -sceneData.directionalLightDir.xyz;
    
    /* Normal mapping */
    mat3 TBN = mat3(fragWorldTangent, fragWorldBiTangent, fragWorldNormal);
    vec3 N = texture(materialTextures[3], fragUV).rgb;
    N = N * 2.0 - 1.0;
    N = normalize(TBN * N);
    
	vec3 albedo = materialData.albedo.rgb * texture(materialTextures[0], fragUV).rgb;
    float ao = materialData.metallicRoughnessAOEmissive.b * texture(materialTextures[1], fragUV).r;
    float emissive = materialData.metallicRoughnessAOEmissive.a * texture(materialTextures[2], fragUV).r;
	
	vec3 irradiance = texture(skyboxIrradiance, N).rgb;
    vec3 diffuse    = irradiance * albedo;
	vec3 kS = fresnelSchlick(max(dot(N, V), 0.0), vec3(0.04));
	vec3 kD = 1.0 - kS;
    vec3 ambient = ao * kD * diffuse;

    vec3 Lo = sceneData.directionalLightColor.xyz * albedo * max(dot(N, L), 0.0);
    vec3 emission = albedo * emissive;
    
    vec3 color = ambient + Lo + emission;
    
    color = tonemapDefault2(color, sceneData.exposure.r);
	
    outColor = vec4(color, 1);
	outHighlight = pushConsts.selected;
}