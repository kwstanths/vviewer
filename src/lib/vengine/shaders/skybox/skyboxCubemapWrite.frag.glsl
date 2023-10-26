#version 450

#extension GL_GOOGLE_include_directive : enable

#include "../include/environmentMap.glsl"

layout (location = 0) out vec4 outColor;
layout (location = 0) in vec3 direction;

layout(set = 0, binding = 0) uniform sampler2D skybox;

void main()
{    
    outColor = texture(skybox, sampleEquirectangularMap(normalize(direction)));
}