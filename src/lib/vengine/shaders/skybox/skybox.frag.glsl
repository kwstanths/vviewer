#version 450

#extension GL_GOOGLE_include_directive : enable

#include "../include/environmentMap.glsl"
#include "../include/tonemapping.glsl"
#include "../include/structs.glsl"

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outHighlight;
layout(location = 0) in vec3 direction;

layout(set = 0, binding = 0) uniform readonly SceneDataUBO {
    SceneData data;
} sceneData;

layout(set = 1, binding = 0) uniform samplerCube skybox;

void main()
{
    outColor.xyz = tonemapDefault2(textureLod(skybox, normalize(direction), 0).xyz, sceneData.data.exposure.r);
	outColor.a = 0;
	outHighlight = vec4(0., 0., 0., 0.);
}