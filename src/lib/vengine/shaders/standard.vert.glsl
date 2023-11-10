#version 450

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

layout(set = 0, binding = 0) uniform readonly SceneDataUBO {
    SceneData data;
} sceneData;

layout(set = 1, binding = 0) uniform readonly ModelDataDescriptor {
	ModelData data[1000];
} modelData;

layout(push_constant) uniform PushConsts {
	layout (offset = 0) vec4 selected;
    layout (offset = 16) uvec4 info;
    layout (offset = 32) uvec4 lights;
} pushConsts;

void main() {
    ModelData m = modelData.data[nonuniformEXT(pushConsts.info.g)];
    vec4 worldPos = m.model * vec4(inPosition, 1.0);
    gl_Position = sceneData.data.projection * sceneData.data.view * worldPos;
    
    fragPos_world = worldPos.xyz;
    fragNormal_world = normalize(vec3(m.model * vec4(inNormal, 0.0)));
    fragTangent_world = normalize(vec3(m.model * vec4(inTangent, 0.0)));
    fragBiTangent_world = normalize(vec3(m.model * vec4(inBitangent, 0.0)));
    fragTangent_world = normalize(fragTangent_world - dot(fragTangent_world, fragNormal_world) * fragNormal_world);
    fragUV = inUV;
}
