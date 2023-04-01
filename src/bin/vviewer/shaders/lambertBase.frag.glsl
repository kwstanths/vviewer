#version 450

#extension GL_GOOGLE_include_directive : enable

#include "pbr.glsl"
#include "lighting.glsl"
#include "tonemapping.glsl"
#include "utils.glsl"
#include "normalmapping.glsl"
#include "structs.glsl"

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragWorldNormal;
layout(location = 2) in vec3 fragWorldTangent;
layout(location = 3) in vec3 fragWorldBiTangent;
layout(location = 4) in vec2 fragUV;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outHighlight;

layout(set = 0, binding = 0) uniform readonly SceneData {
    Scene data;
} scene;

layout(set = 2, binding = 0) uniform readonly MaterialData {
	Material i[200];
} materials;

#extension GL_EXT_nonuniform_qualifier : enable
layout (set = 3, binding = 0) uniform sampler2D global_textures[];
layout (set = 3, binding = 0) uniform sampler3D global_textures_3d[];

layout(set = 4, binding = 1) uniform samplerCube skyboxIrradiance;

layout(push_constant) uniform PushConsts {
	layout (offset = 0) vec4 selected;
    layout (offset = 16) uvec4 material;
} pushConsts;

void main() {

	vec3 cameraPosition = getCameraPosition(scene.data.viewInverse);
    vec3 V = normalize(cameraPosition - fragWorldPos);

    Material materialData = materials.i[nonuniformEXT(pushConsts.material.r)];

    vec2 tiledUV = materialData.uvTiling.rg * fragUV;

    vec3 L = -scene.data.directionalLightDir.xyz;
    
    vec3 N = applyNormalMap(fragWorldTangent, fragWorldBiTangent, fragWorldNormal, texture(global_textures[nonuniformEXT(materialData.gTexturesIndices2.g)], tiledUV).rgb);
    
	vec3 albedo = materialData.albedo.rgb * texture(global_textures[nonuniformEXT(materialData.gTexturesIndices1.r)], tiledUV).rgb;
    float ao = materialData.metallicRoughnessAOEmissive.b * texture(global_textures[nonuniformEXT(materialData.gTexturesIndices1.a)], tiledUV).r;
    float emissive = materialData.metallicRoughnessAOEmissive.a * texture(global_textures[nonuniformEXT(materialData.gTexturesIndices2.r)], tiledUV).r;
	
	vec3 irradiance = texture(skyboxIrradiance, N).rgb;
    vec3 diffuse    = irradiance * albedo;
	vec3 kS = fresnelSchlick(max(dot(N, V), 0.0), vec3(0.04));
	vec3 kD = 1.0 - kS;
    vec3 ambient = ao * kD * diffuse;

    vec3 Lo = scene.data.directionalLightColor.xyz * albedo * max(dot(N, L), 0.0) * INV_PI;
    vec3 emission = albedo * emissive;
    
    vec3 color = scene.data.exposure.g * ambient + Lo + emission;
    
    color = tonemapDefault2(color, scene.data.exposure.r);
	
    outColor = vec4(color, 1);
	outHighlight = pushConsts.selected;
}
