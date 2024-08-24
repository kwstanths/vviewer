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
    layout (offset = 16) uvec4 info;
    layout (offset = 32) uvec4 lights;
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
    
    /* Calculate ambient IBL */
    vec3 ambient = vec3(0.0);
    if (materialType == 0)
    {
        /* PBR STANDARD */
        ambient = ao * calculateIBLContribution(pbr, frame.normal, V_world, skyboxIrradiance, skyboxPrefiltered, global_textures[material.gTexturesIndices2.b]);
    } else if (materialType == 2)
    {
        /* LAMBERT */
        ambient = ao * calculateIBLContribution(pbr.albedo, frame.normal, V_world, skyboxIrradiance);
    }
    vec3 color = sceneData.data.exposure.g * ambient;
    
    outColor = vec4(color, 1.0);
}
