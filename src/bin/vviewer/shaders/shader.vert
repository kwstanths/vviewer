#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragUV;

layout(set = 0, binding = 0) uniform CameraData {
    mat4 view;
    mat4 projection;
} cameraData;

layout(set = 0, binding = 1) uniform ModelData {
    mat4 model;
} modelData;

void main() {
    gl_Position = cameraData.projection * cameraData.view * modelData.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragUV = inUV;
}
