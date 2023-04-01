#version 450

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "pbr.glsl"
#include "skybox/ibl.glsl"
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

layout (set = 3, binding = 0) uniform sampler2D global_textures[];
layout (set = 3, binding = 0) uniform sampler3D global_textures_3d[];

layout(set = 4, binding = 1) uniform samplerCube skyboxIrradiance;
layout(set = 4, binding = 2) uniform samplerCube skyboxPrefiltered;

layout(push_constant) uniform PushConsts {
	layout (offset = 0) vec4 selected;
    layout (offset = 16) uvec4 material;
} pushConsts;

void main() {

    vec3 L = -scene.data.directionalLightDir.xyz;

    Material materialData = materials.i[nonuniformEXT(pushConsts.material.r)];

    vec2 tiledUV = materialData.uvTiling.rg * fragUV;
    
    vec3 N = applyNormalMap(fragWorldTangent, fragWorldBiTangent, fragWorldNormal, texture(global_textures[nonuniformEXT(materialData.gTexturesIndices2.g)], tiledUV).rgb);
    
    vec3 cameraPosition = getCameraPosition(scene.data.viewInverse);
    vec3 V = normalize(cameraPosition - fragWorldPos);
    vec3 H = normalize(V + L);
    
    /* Calculate PBR components */
    PBRStandard pbr;
    pbr.albedo = materialData.albedo.rgb * texture(global_textures[nonuniformEXT(materialData.gTexturesIndices1.r)], tiledUV).rgb;
    pbr.metallic = materialData.metallicRoughnessAOEmissive.r * texture(global_textures[nonuniformEXT(materialData.gTexturesIndices1.g)], tiledUV).r;
    pbr.roughness = materialData.metallicRoughnessAOEmissive.g * texture(global_textures[nonuniformEXT(materialData.gTexturesIndices1.b)], tiledUV).r;
    float ao = materialData.metallicRoughnessAOEmissive.b * texture(global_textures[nonuniformEXT(materialData.gTexturesIndices1.a)], tiledUV).r;
    float emissive = materialData.metallicRoughnessAOEmissive.a * texture(global_textures[nonuniformEXT(materialData.gTexturesIndices2.r)], tiledUV).r;
    
    vec3 ambient = ao * calculateIBLContribution(pbr, N, V, skyboxIrradiance, skyboxPrefiltered, global_textures[nonuniformEXT(materialData.gTexturesIndices2.b)]);
    vec3 Lo = scene.data.directionalLightColor.xyz * calculatePBRStandardShading(pbr, fragWorldPos, N, V, L, H);
    vec3 emission = pbr.albedo * emissive;
    
    vec3 color = scene.data.exposure.g * ambient + Lo + emission;
    
    color = tonemapDefault2(color, scene.data.exposure.r);
	
    outColor = vec4(color, 1);
	outHighlight = pushConsts.selected;
}
