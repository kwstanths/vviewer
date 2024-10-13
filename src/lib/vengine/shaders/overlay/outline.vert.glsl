#version 450

#extension GL_GOOGLE_include_directive : enable
#include "../include/structs.glsl"
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;  

layout(set = 0, binding = 0) uniform readonly SceneDataUBO {
    SceneData data;
} sceneData;

layout(push_constant) uniform PushConsts {
    layout (offset = 0) mat4 modelMatrix;
    layout (offset = 64) vec4 color;
} pushConsts;

void main() {
    vec4 worldPos = pushConsts.modelMatrix * vec4(inPosition + normalize(inNormal) * pushConsts.color.a, 1.0);
    gl_Position = sceneData.data.projection * sceneData.data.view * worldPos;
}
