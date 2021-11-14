#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec3 direction;

layout(set = 0, binding = 0) uniform CameraData {
    mat4 view;
    mat4 projection;
} cameraData;

void main()
{
    /* 
        Remove translation from the view matrix, i guess we could do that on the cpu
        but we would have to setup a new set of descriptors...
    */
    mat4 view = mat4(mat3(cameraData.view));
    
    direction = inPosition;
    gl_Position = cameraData.projection * view * vec4(inPosition, 1);
}  