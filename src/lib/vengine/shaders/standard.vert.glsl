#version 460

#extension GL_GOOGLE_include_directive : enable
#include "include/structs.glsl"
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;  

layout(location = 0) out vec3 fragPos_world;
layout(location = 1) out vec3 fragNormal_world;
layout(location = 2) out vec3 fragTangent_world;
layout(location = 3) out vec3 fragBiTangent_world;
layout(location = 4) out vec2 fragUV;
layout(location = 5) out uint instanceDataIndex;

layout(set = 0, binding = 0) uniform readonly SceneDataUBO {
    SceneData data;
} sceneData;

layout(set = 1, binding = 0) buffer readonly InstanceDataDescriptor {
    InstanceData data[16384];
} instanceData;

void main() {
    instanceDataIndex = gl_InstanceIndex;
    InstanceData instance = instanceData.data[nonuniformEXT(instanceDataIndex)];

    vec4 worldPos = instance.model * vec4(inPosition, 1.0);
    gl_Position = sceneData.data.projection * sceneData.data.view * worldPos;
    
    mat4 invTransModel = transpose(inverse(instance.model));

    fragPos_world = worldPos.xyz;
    fragNormal_world = normalize(vec3(invTransModel * vec4(inNormal, 0.0)));
    fragTangent_world = normalize(vec3(invTransModel * vec4(inTangent, 0.0)));
    fragTangent_world = normalize(fragTangent_world - dot(fragTangent_world, fragNormal_world) * fragNormal_world);
    fragBiTangent_world = cross(fragNormal_world, fragTangent_world);
    fragUV = inUV;
}
