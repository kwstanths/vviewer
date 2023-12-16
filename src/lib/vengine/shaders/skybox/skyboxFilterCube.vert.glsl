#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec3 direction;

layout(push_constant) uniform PushConsts {
    layout (offset = 0) mat4 mvp;
} pushConsts;

void main() 
{
    direction = inPosition;
    gl_Position = pushConsts.mvp * vec4(inPosition.xyz, 1.0);
}