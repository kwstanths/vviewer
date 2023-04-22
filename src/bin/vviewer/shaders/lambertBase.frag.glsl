#version 450

#extension GL_GOOGLE_include_directive : enable

#include "include/constants.glsl"
#include "include/pbr.glsl"
#include "include/lighting.glsl"
#include "include/tonemapping.glsl"
#include "include/utils.glsl"
#include "include/structs.glsl"
#include "include/frame.glsl"

layout(location = 0) in vec3 fragPos_world;
layout(location = 1) in vec3 fragNormal_world;
layout(location = 2) in vec3 fragTangent_world;
layout(location = 3) in vec3 fragBiTangent_world;
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

    vec3 L_world = -scene.data.directionalLightDir.xyz;
	vec3 cameraPosition_world = getCameraPosition(scene.data.viewInverse);
    vec3 V_world = normalize(cameraPosition_world - fragPos_world);

    Material materialData = materials.i[nonuniformEXT(pushConsts.material.r)];
    vec2 tiledUV = materialData.uvTiling.rg * fragUV;

    /* Construct fragment frame */
    Frame frame;
    frame.normal = fragNormal_world;
    frame.tangent = fragTangent_world;
    frame.bitangent = fragBiTangent_world;
    
    /* Normal mapping */
    vec3 newNormal = texture(global_textures[nonuniformEXT(materialData.gTexturesIndices2.g)], tiledUV).rgb;
    applyNormalToFrame(frame, processNormalFromNormalMap(newNormal));

    vec3 L = worldToLocal(frame, L_world);
    vec3 V = worldToLocal(frame, V_world);
    
    /* Calculate material data */
	vec3 albedo = materialData.albedo.rgb * texture(global_textures[nonuniformEXT(materialData.gTexturesIndices1.r)], tiledUV).rgb;
    float ao = materialData.metallicRoughnessAOEmissive.b * texture(global_textures[nonuniformEXT(materialData.gTexturesIndices1.a)], tiledUV).r;
    float emissive = materialData.metallicRoughnessAOEmissive.a * texture(global_textures[nonuniformEXT(materialData.gTexturesIndices2.r)], tiledUV).r;
	
	vec3 irradiance = texture(skyboxIrradiance, frame.normal).rgb;
    vec3 diffuse    = irradiance * albedo;
	vec3 kS = fresnelSchlick(max(V.y, 0.0), vec3(0.04));
	vec3 kD = 1.0 - kS;
    vec3 ambient = ao * kD * diffuse;

    vec3 Lo = scene.data.directionalLightColor.xyz * albedo * max(L.y, 0.0) * INVPI;
    vec3 emission = albedo * emissive;
    
    vec3 color = scene.data.exposure.g * ambient + Lo + emission;
    
    color = tonemapDefault2(color, scene.data.exposure.r);
	
    outColor = vec4(color, 1);
	outHighlight = pushConsts.selected;
}
