#version 450

#extension GL_GOOGLE_include_directive : enable

#include "include/brdfs/common.glsl"
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

layout(push_constant) uniform PushConsts {
	layout (offset = 0) vec4 lightPosition;
    layout (offset = 16) vec4 lightColor;
    layout (offset = 32) uvec4 material;
} pushConsts;

void main() {
    vec3 L_world = pushConsts.lightPosition.rgb;
    vec3 L_color = pushConsts.lightColor.rgb;

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

    vec3 L = worldToLocal(frame, normalize(L_world - fragPos_world));

    float attenuation = squareDistanceAttenuation(fragPos_world, L_world);
    /* If contribution of light is smaller than 0.05 ignore it. Since we don't have a light radius right now to limit it */
    if (attenuation * max3(L_color) < 0.05)
    {
        outColor = vec4(0, 0, 0, 1);
        outHighlight = vec4(0, 0, 0, 0);
        return;
    }
    
    vec3 albedo = materialData.albedo.rgb * texture(global_textures[nonuniformEXT(materialData.gTexturesIndices1.r)], tiledUV).rgb;
    
    vec3 Lo = L_color * albedo * max(L.y, 0.0) * INV_PI * attenuation;
    
    vec3 color = Lo;
    
    color = tonemapDefault2(color, scene.data.exposure.r);
	
    outColor = vec4(color, 1);
    outHighlight = vec4(0, 0, 0, 0);
}
