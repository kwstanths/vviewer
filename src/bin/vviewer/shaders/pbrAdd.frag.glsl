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

layout(push_constant) uniform PushConsts {
	layout (offset = 0) vec4 lightPosition;
    layout (offset = 16) vec4 lightColor;
    layout (offset = 32) uvec4 material;
} pushConsts;

void main() {
    vec3 lightWorldPos = pushConsts.lightPosition.rgb;
    vec3 lightColor = pushConsts.lightColor.rgb;

    Material materialData = materials.i[nonuniformEXT(pushConsts.material.r)];

    vec2 tiledUV = materialData.uvTiling.rg * fragUV;
    
    vec3 L = normalize(lightWorldPos - fragWorldPos);
    float attenuation = squareDistanceAttenuation(fragWorldPos, lightWorldPos);
    /* If contribution of light is smaller than 0.05 ignore it. Since we don't have a light radius right now to limit it */
    if (attenuation * max3(lightColor) < 0.05)
    {
        outColor = vec4(0, 0, 0, 1);
    	outHighlight = vec4(0, 0, 0, 0);
        return;
    }
    
    vec3 N = applyNormalMap(fragWorldTangent, fragWorldBiTangent, fragWorldNormal, texture(global_textures[nonuniformEXT(materialData.gTexturesIndices2.g)], tiledUV).rgb);
    
    vec3 cameraPosition = getCameraPosition(scene.data.viewInverse);
    vec3 V = normalize(cameraPosition - fragWorldPos);
    vec3 H = normalize(V + L);
    
    /* Calculate PBR components */
    PBRStandard pbr;
    pbr.albedo = materialData.albedo.rgb * texture(global_textures[nonuniformEXT(materialData.gTexturesIndices1.r)], tiledUV).rgb;
    pbr.metallic = materialData.metallicRoughnessAOEmissive.r * texture(global_textures[nonuniformEXT(materialData.gTexturesIndices1.g)], tiledUV).r;
    pbr.roughness = materialData.metallicRoughnessAOEmissive.g * texture(global_textures[nonuniformEXT(materialData.gTexturesIndices1.b)], tiledUV).r;  
    vec3 Lo = lightColor * calculatePBRStandardShading(pbr, fragWorldPos, N, V, L, H) * attenuation;
    
    vec3 color = Lo;
    
    color = tonemapDefault2(color, scene.data.exposure.r);
	
    outColor = vec4(color, 1);
	outHighlight = vec4(0, 0, 0, 0);
}
