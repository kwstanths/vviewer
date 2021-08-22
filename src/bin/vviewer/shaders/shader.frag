#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 2) uniform sampler2D texSampler;

void main() {
    //outColor = vec4(texture(texSampler, fragUV).rgb, 1);
    outColor = vec4(fragNormal, 1);
}