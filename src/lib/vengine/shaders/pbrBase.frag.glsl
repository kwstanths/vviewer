#version 450

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "include/brdfs/pbrStandard.glsl"
#include "skybox/ibl.glsl"
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

layout(set = 0, binding = 0) uniform readonly SceneDataUBO {
    SceneData data;
} sceneData;

layout(set = 1, binding = 0) uniform readonly ModelDataDescriptor {
    ModelData data[1000];
} modelData;

layout(set = 2, binding = 0) uniform readonly LightDataUBO {
    LightData data[200];
} lightData;

layout(set = 2, binding = 1) uniform readonly LightComponentsUBO {
    LightComponent data[1000];
} lightComponents;

layout(set = 3, binding = 0) uniform readonly MaterialDataUBO {
    MaterialData data[200];
} materialData;

layout (set = 4, binding = 0) uniform sampler2D global_textures[];
layout (set = 4, binding = 0) uniform sampler3D global_textures_3d[];

layout(set = 5, binding = 1) uniform samplerCube skyboxIrradiance;
layout(set = 5, binding = 2) uniform samplerCube skyboxPrefiltered;

layout(push_constant) uniform PushConsts {
    layout (offset = 0) vec4 selected;
    layout (offset = 16) uvec4 info;
    layout (offset = 32) uvec4 lights;
} pushConsts;

void main() {

    vec3 cameraPosition_world = getTranslation(sceneData.data.viewInverse);
    vec3 V_world = normalize(cameraPosition_world - fragPos_world);

    MaterialData material = materialData.data[nonuniformEXT(pushConsts.info.r)];
    vec2 tiledUV = material.uvTiling.rg * fragUV;

    /* Construct fragment frame */
    Frame frame;
    frame.normal = fragNormal_world;
    frame.tangent = fragTangent_world;
    frame.bitangent = fragBiTangent_world;

    /* Normal mapping */
    vec3 newNormal = texture(global_textures[nonuniformEXT(material.gTexturesIndices2.g)], tiledUV).rgb;
    applyNormalToFrame(frame, processNormalFromNormalMap(newNormal));
    
    /* Calculate PBR data */
    PBRStandard pbr;
    pbr.albedo = material.albedo.rgb * texture(global_textures[nonuniformEXT(material.gTexturesIndices1.r)], tiledUV).rgb;
    pbr.metallic = material.metallicRoughnessAO.r * texture(global_textures[nonuniformEXT(material.gTexturesIndices1.g)], tiledUV).r;
    pbr.roughness = material.metallicRoughnessAO.g * texture(global_textures[nonuniformEXT(material.gTexturesIndices1.b)], tiledUV).r;
    float ao = material.metallicRoughnessAO.b * texture(global_textures[nonuniformEXT(material.gTexturesIndices1.a)], tiledUV).r;
    vec3 emissive = material.emissive.a * material.emissive.rgb * texture(global_textures[nonuniformEXT(material.gTexturesIndices2.r)], tiledUV).rgb;
    float alpha = material.albedo.a * texture(global_textures[nonuniformEXT(material.gTexturesIndices2.a)], tiledUV).r;
    
    /* Calculate ambient IBL */
    vec3 ambient = ao * calculateIBLContribution(pbr, frame.normal, V_world, skyboxIrradiance, skyboxPrefiltered, global_textures[nonuniformEXT(material.gTexturesIndices2.b)]);

    /* Calculate direct light */
    vec3 direct = vec3(0);
    uint lights[4] = {pushConsts.lights.r, pushConsts.lights.g, pushConsts.lights.b, pushConsts.lights.a};
    uint totalLights = pushConsts.info.b;
    for(uint i = 0; i < 4; i++)
    {
        if (i >= totalLights)
        {
            continue;
        }

        uint lightIndex = lights[i];

        LightComponent lc = lightComponents.data[nonuniformEXT(lightIndex)];
        LightData ld = lightData.data[nonuniformEXT(lc.info.r)];
        ModelData md = modelData.data[nonuniformEXT(lc.info.g)];

        vec3 L_color = ld.color.rgb * ld.color.a;
        if (!isBlack(L_color))
        {
            if (ld.type.r == 0)
            {
                /* Point light */
                vec3 L_world = getTranslation(md.model);

                vec3 L = worldToLocal(frame, normalize(L_world - fragPos_world));
                vec3 V = worldToLocal(frame, V_world);
                vec3 H = normalize(V + L);

                /* Calculate light contrubution from point light */
                float attenuation = squareDistanceAttenuation(fragPos_world, L_world);
                /* If contribution of light is smaller than 0.05 ignore it. Since we don't have a light radius right now to limit it */
                if (attenuation * max3(L_color) < 0.05)
                {
                    continue;
                }
                direct += L_color * evalPBRStandard(pbr, L, V, H) * attenuation;

            } else if (ld.type.r == 1) {
                vec3 L_world = vec3(md.model * vec4(0, 0, 1, 0));
                /* Calculate light contrubution from directional light */
                vec3 L = worldToLocal(frame, normalize(-L_world));
                vec3 V = worldToLocal(frame, V_world);
                vec3 H = normalize(V + L);

                direct += L_color * evalPBRStandard(pbr, L, V, H);
            }
        }
    }

    /* Add all together */
    vec3 color = sceneData.data.exposure.g * ambient + direct + emissive;
    
    color = tonemapDefault2(color, sceneData.data.exposure.r);
    
    outColor = vec4(color, alpha);
    outHighlight = pushConsts.selected;
}
