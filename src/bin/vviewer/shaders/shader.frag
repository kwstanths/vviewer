#version 450

#include "pbr.glsl"

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform CameraData {
    mat4 view;
    mat4 projection;
} cameraData;

layout(set = 0, binding = 2) uniform sampler2D texSampler;

vec3 getCameraPosition(mat4 invViewMatrix)
{
    return vec3(invViewMatrix[3][0], invViewMatrix[3][1], invViewMatrix[3][2]);
}

void main() {

    vec3 lightColor = vec3(20, 20, 20);
    vec3 lightPosition = vec3(3, 3, 3);
    vec3 L = normalize(lightPosition - fragWorldPos);
    
    vec3 cameraPosition = getCameraPosition(inverse(cameraData.view));
    vec3 V = normalize(cameraPosition - fragWorldPos);

    vec3 H = normalize(V + L);
    
    float distance    = length(lightPosition - fragWorldPos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance     = lightColor * attenuation; 
    
    PBRStandard pbr;
    pbr.albedo = vec3(1, 1, 1);
    pbr.metallic = 0.4;
    pbr.roughness = 0.1;
    float ao = 0.3;
    
    vec3 ambient = vec3(0.03) * pbr.albedo * ao;
    outColor =  vec4(ambient + radiance * calculatePBRStandardShading(pbr, fragWorldPos, fragNormal, V, L, H), 1);
}