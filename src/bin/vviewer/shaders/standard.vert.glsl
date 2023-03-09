#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;  

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragWorldNormal;
layout(location = 2) out vec3 fragWorldTangent;
layout(location = 3) out vec3 fragWorldBiTangent;
layout(location = 4) out vec2 fragUV;

layout(set = 0, binding = 0) uniform SceneData {
    mat4 view;
	mat4 viewInverse;
    mat4 projection;
	mat4 projectionInverse;
    vec4 directionalLightDir;
    vec4 directionalLightColor;
    vec4 exposure;
} sceneData;

layout(set = 1, binding = 0) uniform ModelData {
    mat4 model;
} modelData;

void main() {
    vec4 worldPos = modelData.model * vec4(inPosition, 1.0);
    gl_Position = sceneData.projection * sceneData.view * worldPos;
    
    fragWorldPos = worldPos.xyz;
    fragWorldNormal = normalize(vec3(modelData.model * vec4(inNormal, 0.0)));
    fragWorldTangent = normalize(vec3(modelData.model * vec4(inTangent, 0.0)));
    fragWorldBiTangent = normalize(vec3(modelData.model * vec4(inBitangent, 0.0)));
    fragWorldTangent = normalize(fragWorldTangent - dot(fragWorldTangent, fragWorldNormal) * fragWorldNormal);
    fragUV = inUV;
}
