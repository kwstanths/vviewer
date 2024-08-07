#version 450

#extension GL_GOOGLE_include_directive : enable
#include "../include/structs.glsl"

layout(location = 0) in vec3 inPosition;

layout(set = 0, binding = 0) uniform readonly SceneDataUBO {
    SceneData data;
} sceneData;

layout(push_constant) uniform PushConsts {
    layout (offset = 0) mat4 modelMatrix;
} pushConsts;

void main() {
    vec4 worldPos = pushConsts.modelMatrix * vec4(inPosition, 1.0);
    gl_Position = sceneData.data.projection * sceneData.data.view * worldPos;
}
