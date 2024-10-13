#version 450

#extension GL_GOOGLE_include_directive : enable

#include "../include/structs.glsl"

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outID;

layout(set = 0, binding = 0) uniform readonly SceneDataUBO {
    SceneData data;
} sceneData;

layout(push_constant) uniform PushConsts {
    layout (offset = 64) vec4 color;
} pushConsts;

void main() {

    vec3 color = pushConsts.color.rgb;
    
    outColor = vec4(color, 1);
    outID = vec4(0, 0, 0, pushConsts.color.a);
}