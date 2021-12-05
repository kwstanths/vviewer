#version 450

#include "environmentMap.glsl"

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec3 direction;

layout(set = 1, binding = 0) uniform samplerCube skybox;

void main()
{
    outColor = texture(skybox, normalize(direction));
}