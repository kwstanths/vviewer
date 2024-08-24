#version 450

#extension GL_GOOGLE_include_directive : enable
#include "../include/structs.glsl"

layout (points) in;
layout (line_strip, max_vertices = 16) out;

layout(set = 0, binding = 0) uniform readonly SceneDataUBO {
    SceneData data;
} sceneData;

layout(push_constant) uniform PushConsts {
    layout (offset = 0) vec4 minp;
    layout (offset = 16) vec4 maxp;
} pushConsts;

void main(void)
{	
        /* Create a line strip for an AABB given the min and pax points */
        vec3 pos = gl_in[0].gl_Position.xyz;	/* min position */
        
        vec4 diff = pushConsts.maxp - pushConsts.minp;

        mat4 viewProjection = sceneData.data.projection * sceneData.data.view;

        gl_Position = viewProjection * vec4(pos, 1.0);
        EmitVertex();

        pos += vec3(diff.x, 0, 0);
        gl_Position = viewProjection * vec4(pos, 1.0);
        EmitVertex();

        pos += vec3(0, 0, diff.z);
        gl_Position = viewProjection * vec4(pos, 1.0);
        EmitVertex();

        pos += vec3(-diff.x, 0, 0);
        gl_Position = viewProjection * vec4(pos, 1.0);
        EmitVertex();

        pos += vec3(0, 0, -diff.z);
        gl_Position = viewProjection * vec4(pos, 1.0);
        EmitVertex();

        pos += vec3(0, diff.y, 0);
        gl_Position = viewProjection * vec4(pos, 1.0);
        EmitVertex();

        pos += vec3(diff.x, 0, 0);
        gl_Position = viewProjection * vec4(pos, 1.0);
        EmitVertex();

        pos += vec3(0, -diff.y, 0);
        gl_Position = viewProjection * vec4(pos, 1.0);
        EmitVertex();

        pos += vec3(0, diff.y, 0);
        gl_Position = viewProjection * vec4(pos, 1.0);
        EmitVertex();

        pos += vec3(0, 0, diff.z);
        gl_Position = viewProjection * vec4(pos, 1.0);
        EmitVertex();

        pos += vec3(0, -diff.y, 0);
        gl_Position = viewProjection * vec4(pos, 1.0);
        EmitVertex();

        pos += vec3(0, diff.y, 0);
        gl_Position = viewProjection * vec4(pos, 1.0);
        EmitVertex();

        pos += vec3(-diff.x, 0, 0);
        gl_Position = viewProjection * vec4(pos, 1.0);
        EmitVertex();

        pos += vec3(0, -diff.y, 0);
        gl_Position = viewProjection * vec4(pos, 1.0);
        EmitVertex();

        pos += vec3(0, diff.y, 0);
        gl_Position = viewProjection * vec4(pos, 1.0);
        EmitVertex();

        pos += vec3(0, 0, -diff.z);
        gl_Position = viewProjection * vec4(pos, 1.0);
        EmitVertex();

        EndPrimitive();
}