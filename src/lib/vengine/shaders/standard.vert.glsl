#version 450

#extension GL_GOOGLE_include_directive : enable
#include "include/structs.glsl"

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

layout(set = 0, binding = 0) uniform readonly SceneData {
    Scene data;
} scene;

layout(set = 1, binding = 0) uniform ModelData {
    mat4 model;
} modelData;

void main() {
    vec4 worldPos = modelData.model * vec4(inPosition, 1.0);
    gl_Position = scene.data.projection * scene.data.view * worldPos;
    
    fragPos_world = worldPos.xyz;
    fragNormal_world = normalize(vec3(modelData.model * vec4(inNormal, 0.0)));
    fragTangent_world = normalize(vec3(modelData.model * vec4(inTangent, 0.0)));
    fragBiTangent_world = normalize(vec3(modelData.model * vec4(inBitangent, 0.0)));
    fragTangent_world = normalize(fragTangent_world - dot(fragTangent_world, fragNormal_world) * fragNormal_world);
    fragUV = inUV;
}