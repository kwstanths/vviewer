#version 450

#include "tonemapping.glsl"

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform SceneData {
    mat4 view;
	mat4 viewInverse;
    mat4 projection;
	mat4 projectionInverse;
    vec4 directionalLightDir;
    vec4 directionalLightColor;
    vec4 exposure;
} sceneData;

layout(push_constant) uniform PushConsts {
	layout (offset = 64) vec4 color;
} pushConsts;

void main() {

    vec3 color = pushConsts.color.rgb;
    
    color = tonemapDefault2(color, sceneData.exposure.r);
	
    outColor = vec4(color, 1);
}