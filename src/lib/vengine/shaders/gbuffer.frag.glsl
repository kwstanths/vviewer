#version 460

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "include/structs.glsl"
#include "include/frame.glsl"
#include "include/utils.glsl"
#include "include/packing.glsl"

layout(location = 0) in vec3 fragPos_world;
layout(location = 1) in vec3 fragNormal_world;
layout(location = 2) in vec3 fragTangent_world;
layout(location = 3) in vec3 fragBiTangent_world;
layout(location = 4) in vec2 fragUV;
layout(location = 5) flat in uint instanceDataIndex;

layout(location = 0) out vec4 outGBuffer1;
layout(location = 1) out uvec4 outGBuffer2;
layout(location = 2) out vec4 outColor;

layout(set = 1, binding = 0) buffer readonly InstanceDataUBO {
    InstanceData data[16384];
} instanceData;

layout(set = 2, binding = 0) uniform readonly MaterialDataUBO {
    MaterialData data[512];
} materialData;

layout (set = 3, binding = 0) uniform sampler2D global_textures[];
layout (set = 3, binding = 0) uniform sampler3D global_textures_3d[];

void main() {
    InstanceData instance = instanceData.data[nonuniformEXT(instanceDataIndex)];

    uint materialIndex = nonuniformEXT(instance.materialIndex);
    MaterialData material = materialData.data[nonuniformEXT(instance.materialIndex)];
    vec2 tiledUV = material.uvTiling.rg * fragUV;

    /* Construct a frame */
    Frame frame;
    frame.normal = normalize(fragNormal_world);
    frame.tangent = normalize(fragTangent_world);
    frame.bitangent = normalize(fragBiTangent_world);

    /* Apply normal mapping on frame */
    vec3 newNormal = texture(global_textures[nonuniformEXT(material.gTexturesIndices2.g)], tiledUV).rgb;
    applyNormalToFrame(frame, processNormalFromNormalMap(newNormal));

    /* Material data */
    vec3 albedo = material.albedo.rgb * texture(global_textures[nonuniformEXT(material.gTexturesIndices1.r)], tiledUV).rgb;
    float metallic = material.metallicRoughnessAO.r * texture(global_textures[nonuniformEXT(material.gTexturesIndices1.g)], tiledUV).r;
    float roughness = material.metallicRoughnessAO.g * texture(global_textures[nonuniformEXT(material.gTexturesIndices1.b)], tiledUV).r;
    float ao = material.metallicRoughnessAO.b * texture(global_textures[nonuniformEXT(material.gTexturesIndices1.a)], tiledUV).r;
    vec3 emissive = material.emissive.a * material.emissive.rgb * texture(global_textures[nonuniformEXT(material.gTexturesIndices2.r)], tiledUV).rgb;

    outGBuffer1 = vec4(frame.normal, instance.id.r);
    outGBuffer2 = uvec4(pack8bitTo16bit(uint(255 * albedo.r), uint(255 * albedo.g)), pack8bitTo16bit(uint(255 * albedo.b), uint(255 * ao)), pack8bitTo16bit(uint(255 * metallic), uint(255 * roughness)), materialIndex);
    outColor = vec4(emissive, 1.0);
}