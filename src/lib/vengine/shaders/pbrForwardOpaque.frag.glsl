#version 460

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_ray_query : enable

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
layout(location = 5) flat in uint instanceDataIndex;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outHighlight;

layout(set = 0, binding = 0) uniform readonly SceneDataUBO {
    SceneData data;
} sceneData;

layout(set = 1, binding = 0) buffer readonly InstanceDataUBO {
    InstanceData data[16384];
} instanceData;

layout(set = 2, binding = 0) uniform readonly LightDataUBO {
    LightData data[1024];
} lightData;

layout(set = 2, binding = 1) uniform readonly LightInstancesUBO {
    LightInstance data[1024];
} lightInstances;

layout(set = 3, binding = 0) uniform readonly MaterialDataUBO {
    MaterialData data[512];
} materialData;

layout (set = 4, binding = 0) uniform sampler2D global_textures[];
layout (set = 4, binding = 0) uniform sampler3D global_textures_3d[];

layout(set = 5, binding = 1) uniform samplerCube skyboxIrradiance;
layout(set = 5, binding = 2) uniform samplerCube skyboxPrefiltered;

layout(set = 6, binding = 0) uniform accelerationStructureEXT topLevelAS;

layout(push_constant) uniform PushConsts {
    layout (offset = 16) uvec4 info;
    layout (offset = 32) uvec4 lights;
} pushConsts;

void main() {
    InstanceData instance = instanceData.data[nonuniformEXT(instanceDataIndex)];

    vec3 cameraPosition_world = getTranslation(sceneData.data.viewInverse);
    vec3 V_world = normalize(cameraPosition_world - fragPos_world);

    MaterialData material = materialData.data[nonuniformEXT(instance.materialIndex)];
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

        LightInstance lc = lightInstances.data[nonuniformEXT(lightIndex)];
        LightData ld = lightData.data[nonuniformEXT(lc.info.r)];
        
        vec3 L_color = ld.color.rgb * ld.color.a;
        if (!isBlack(L_color))
        {
            if (ld.type.r == 0)
            {
                /* Point light */
                vec3 L_world_position = lc.position.rgb;
                vec3 L_world_direction = L_world_position - fragPos_world;
                float L_distance = length(L_world_direction);
                vec3 L_world_direction_normalized = normalize(L_world_direction); 

                vec3 L = worldToLocal(frame, L_world_direction_normalized);
                vec3 V = worldToLocal(frame, V_world);
                vec3 H = normalize(V + L);

                float attenuation = squareDistanceAttenuation(L_distance);
                /* If contribution of light is smaller than 0.025 ignore it. Since we don't have a light radius right now to limit it */
                if (attenuation * max3(L_color) < 0.025)
                {
                    continue;
                }

                /* trace shadow ray */
                float diffuseStrength = 1.0;
                float specularStrength = 1.0;
                if (lc.position.a == 1.0)
                {
                    vec3 rayquery_direction = L_world_direction_normalized;
                    float rayquery_distance = L_distance;
                    #include "include/rayquery_occluded.glsl"
                    if (occluded)
                    {
                        diffuseStrength = 0.05; /* Add 5% ambient */
                        specularStrength = 0.0;
                    }
                } 
                direct += L_color * evalPBRStandard_mult(pbr, L, V, H, diffuseStrength, specularStrength) * attenuation;

            } else if (ld.type.r == 1) {
                vec3 L_world_direction_normalized = normalize(-lc.position.rgb);

                /* Calculate light contrubution from directional light */
                vec3 L = worldToLocal(frame, L_world_direction_normalized);
                vec3 V = worldToLocal(frame, V_world);
                vec3 H = normalize(V + L);
                
                /* trace shadow raw */
                float diffuseStrength = 1.0;
                float specularStrength = 1.0;
                if (lc.position.a == 1.0)
                {
                    vec3 rayquery_direction = L_world_direction_normalized;
                    float rayquery_distance = 10000.0;
                    #include "include/rayquery_occluded.glsl"
                    if (occluded)
                    {
                        diffuseStrength = 0.05; /* Add 5% ambient */
                        specularStrength = 0.0;
                    }
                }
                direct += L_color * evalPBRStandard_mult(pbr, L, V, H, diffuseStrength, specularStrength);
            }
        }
    }

    /* Add all together */
    vec3 color = sceneData.data.exposure.g * ambient + direct + emissive;
    
    color = tonemapDefault2(color, sceneData.data.exposure.r);
    
    outColor = vec4(color, alpha);
    outHighlight = instance.id;
}
