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
#include "include/packing.glsl"

layout (location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput inputGBuffer1;
layout (input_attachment_index = 1, set = 0, binding = 1) uniform usubpassInput inputGBuffer2;
layout (input_attachment_index = 2, set = 0, binding = 2) uniform subpassInput inputDepth;

layout(set = 1, binding = 0) uniform readonly SceneDataUBO {
    SceneData data;
} sceneData;

layout(set = 2, binding = 0) buffer readonly InstanceDataUBO {
    InstanceData data[16384];
} instanceData;

layout(set = 3, binding = 0) uniform readonly LightDataUBO {
    LightData data[1024];
} lightData;

layout(set = 3, binding = 1) uniform readonly LightInstancesUBO {
    LightInstance data[1024];
} lightInstances;

layout(set = 4, binding = 0) uniform readonly MaterialDataUBO {
    MaterialData data[512];
} materialData;

layout (set = 5, binding = 0) uniform sampler2D global_textures[];
layout (set = 5, binding = 0) uniform sampler3D global_textures_3d[];

layout(set = 6, binding = 1) uniform samplerCube skyboxIrradiance;
layout(set = 6, binding = 2) uniform samplerCube skyboxPrefiltered;

layout(set = 7, binding = 0) uniform accelerationStructureEXT topLevelAS;

layout(push_constant) uniform PushConsts {
    layout (offset = 0) uvec4 lights;
} pushConsts;

void main() {

    /* Read G-Buffer values */
	vec4 gbuffer1 = subpassLoad(inputGBuffer1);
	uvec4 gbuffer2 = subpassLoad(inputGBuffer2);
	float depth = subpassLoad(inputDepth).r;

    /* Calculate geometry */
    vec3 worldPos = worldPositionFromDepth(depth, inUV, sceneData.data.projectionInverse, sceneData.data.viewInverse);
    vec3 normalWorld = gbuffer1.rgb;
    
    /* Discard pixels with invalid geometry */
    if (length(normalWorld) <= 0.95)
    {
        discard;
    }

    /* Construct frame from normal */
    Frame frame = frameFromNormal(normalWorld);

    vec3 cameraPosition_world = getTranslation(sceneData.data.viewInverse);
    vec3 V_world = normalize(cameraPosition_world - worldPos);

    /* Parse material data */
    PBRStandard pbr;
    float ao;
    unpack16bitTo8Bit(uint(gbuffer2.r), pbr.albedo.r, pbr.albedo.g);
    unpack16bitTo8Bit(uint(gbuffer2.g), pbr.albedo.b, ao);
    unpack16bitTo8Bit(uint(gbuffer2.b), pbr.metallic, pbr.roughness);
    uint materialIndex = gbuffer2.a;

    MaterialData material = materialData.data[materialIndex];
    uint materialType = uint(material.uvTiling.b);
    
    /* Calculate direct light */
    vec3 direct = vec3(0);
    uint lightIndex = pushConsts.lights.r;
        
    LightInstance lc = lightInstances.data[nonuniformEXT(lightIndex)];
    LightData ld = lightData.data[nonuniformEXT(lc.info.r)];
        
    vec3 L_color = ld.color.rgb * ld.color.a;
    if (!isBlack(L_color))
    {
        /* Point light */
        vec3 L_world_position = lc.position.rgb;
        vec3 L_world_direction = L_world_position - worldPos;
        float L_distance = length(L_world_direction);
        vec3 L_world_direction_normalized = normalize(L_world_direction);
        
        vec3 L = worldToLocal(frame, L_world_direction_normalized);
        vec3 V = worldToLocal(frame, V_world);
        vec3 H = normalize(V + L);
        
        float attenuation = squareDistanceAttenuation(L_distance);
        
        /* If contribution of light is smaller than 0.025 ignore it. Since we don't have a light radius right now to limit it */
        if (attenuation * max3(L_color) >= 0.025)
        {
            /* trace shadow ray if requested */
            float diffuseStrength = 1.0;
            float specularStrength = 1.0;
            if (lc.position.a == 1.0)
            {
                vec3 rayquery_direction = L_world_direction_normalized;
                float rayquery_distance = L_distance;
                vec3 fragPos_world = worldPos + L_world_direction_normalized * 0.05;
                #include "include/rayquery_occluded.glsl"

                if (occluded)
                {
                    diffuseStrength = 0.05; /* Add 5% ambient */
                    specularStrength = 0.0;
                }
            } 
            
            if (materialType == 0)
            {
                /* PBR STANDARD */
                direct += L_color * evalPBRStandard_mult(pbr, L, V, H, diffuseStrength, specularStrength) * attenuation;
            } else if (materialType == 2)
            {
                /* LAMBERT */
                direct += diffuseStrength * L_color * pbr.albedo * max(L.y, 0.0) * INV_PI * attenuation;
            }
            
        }
    }

    vec3 color = direct;
    outColor = vec4(direct, 1.0);
    
}
