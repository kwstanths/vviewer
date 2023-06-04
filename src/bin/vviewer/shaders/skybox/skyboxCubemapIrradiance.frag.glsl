#version 450

#extension GL_GOOGLE_include_directive : enable

layout (location = 0) out vec4 outColor;
layout (location = 0) in vec3 direction;
layout (set = 0, binding = 0) uniform samplerCube cubemap;

layout(push_constant) uniform PushConsts {
	layout (offset = 64) float deltaPhi;
	layout (offset = 68) float deltaTheta;
} pushConsts;

#include "../include/constants.glsl"

void main()
{
	vec3 N = normalize(direction);
	vec3 up = vec3(0.0, 1.0, 0.0);
	vec3 right = normalize(cross(up, N));
	up = cross(N, right);

	vec3 color = vec3(0.0);
	uint sampleCount = 0u;
	for (float phi = 0.0; phi < TWO_TIMES_PI; phi += pushConsts.deltaPhi) {
		for (float theta = 0.0; theta < PI_OVER_TWO; theta += pushConsts.deltaTheta) {
			vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
			vec3 sampleVector = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;
			color += texture(cubemap, sampleVector).rgb * cos(theta) * sin(theta);
			sampleCount++;
		}
	}
	outColor = vec4(PI * color / float(sampleCount), 1.0);
}