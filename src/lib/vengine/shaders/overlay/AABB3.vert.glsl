#version 450

layout(push_constant) uniform PushConsts {
    layout (offset = 0) vec4 minp;
} pushConsts;

void main() {
    vec4 worldPos = pushConsts.minp;
    gl_Position = worldPos;
}
