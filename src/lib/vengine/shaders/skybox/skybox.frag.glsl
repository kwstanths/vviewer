#version 450

#extension GL_GOOGLE_include_directive : enable

#include "../include/environmentMap.glsl"
#include "../include/tonemapping.glsl"
#include "../include/structs.glsl"

layout(location = 0) out vec4 outGBuffer1;
layout(location = 1) out uvec4 outGBuffer2;
layout(location = 2) out vec4 outColor;
layout(location = 0) in vec3 direction;

layout(set = 0, binding = 0) uniform readonly SceneDataUBO {
    SceneData data;
} sceneData;

layout(set = 1, binding = 0) uniform samplerCube skybox;

void main()
{
    outColor.xyz = textureLod(skybox, normalize(direction), 0).xyz;
    outColor.a = 1.0;
    
    outGBuffer1 = vec4(0., 0., 0., 0.);
    outGBuffer2 = uvec4(0., 0., 0., 0.);
}