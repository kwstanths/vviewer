#version 450

#extension GL_GOOGLE_include_directive : enable

#include "../include/tonemapping.glsl"
#include "../include/structs.glsl"

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outHighlight;

layout(set = 0, binding = 0) uniform readonly SceneDataUBO {
    SceneData data;
} sceneData;

layout(push_constant) uniform PushConsts {
    layout (offset = 32) vec4 color;
    layout (offset = 48) vec4 selected;
} pushConsts;

void main() {

    vec3 color = pushConsts.color.rgb;
    
    color = tonemapDefault2(color, sceneData.data.exposure.r);
    
    outColor = vec4(color, 1);
    outHighlight = pushConsts.selected;
}