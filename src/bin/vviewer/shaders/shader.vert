#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragWorldNormal;
layout(location = 2) out vec2 fragUV;

layout(set = 0, binding = 0) uniform CameraData {
    mat4 view;
    mat4 projection;
} cameraData;

layout(set = 1, binding = 0) uniform ModelData {
    mat4 model;
} modelData;

void main() {
    vec4 worldPos = modelData.model * vec4(inPosition, 1.0);
    gl_Position = cameraData.projection * cameraData.view * worldPos;
    fragWorldPos = worldPos.xyz;
    fragWorldNormal = normalize(mat3(modelData.model) * inNormal);
    fragUV = inUV;
}
