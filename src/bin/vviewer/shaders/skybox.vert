#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec3 direction;

layout(set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 projection;
} sceneData;

void main()
{
    /* 
        Remove translation from the view matrix, i guess we could do that on the cpu
        but we would have to setup a new set of descriptors...
    */
    mat4 view = mat4(mat3(sceneData.view));
    
    direction = inPosition;
    gl_Position = sceneData.projection * view * vec4(inPosition, 1);
}  