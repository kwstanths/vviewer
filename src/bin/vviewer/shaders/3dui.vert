#version 450

layout(location = 0) in vec3 inPosition;

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
	layout (offset = 0) mat4 modelMatrix;
} pushConsts;

void main() {
    vec4 worldPos = pushConsts.modelMatrix * vec4(inPosition, 1.0);
    gl_Position = sceneData.projection * sceneData.view * worldPos;
}
