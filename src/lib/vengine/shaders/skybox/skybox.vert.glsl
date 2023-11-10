#version 450

#extension GL_GOOGLE_include_directive : enable
#include "../include/structs.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec3 direction;

layout(set = 0, binding = 0) uniform readonly SceneDataUBO {
    SceneData data;
} sceneData;

void main()
{
    /* 
        Remove translation from the view matrix, i guess we could do that on the cpu
    */
    mat4 view = mat4(mat3(sceneData.data.view));
    
    direction = inPosition;
    gl_Position = sceneData.data.projection * view * vec4(inPosition, 1);
}  