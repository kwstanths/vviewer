#version 450

#extension GL_GOOGLE_include_directive : enable

#include "../include/structs.glsl"

layout (location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D inputColor;
layout(set = 1, binding = 0) uniform readonly SceneDataUBO {
    SceneData data;
} sceneData;

layout(push_constant) uniform PushConsts {
    layout (offset = 0) vec4 info;
} pushConsts;

#include "../include/tonemapping.glsl"

void main()
{
    float exposure = sceneData.data.exposure.r;

    vec4 color = texture(inputColor, inUV).rgba;

    color.rgb = tonemapDefault2(color.rgb, exposure);

    outColor = color;
}