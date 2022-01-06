#version 450

layout (location = 0) out vec4 outColor;
layout (location = 0) in vec3 direction;
layout (set = 0, binding = 0) uniform samplerCube cubemap;

layout(push_constant) uniform PushConsts {
	layout (offset = 64) float roughness;
	layout (offset = 68) uint numSamples;
} pushConsts;

#include "pbr.glsl"
#include "sampling.glsl"

vec3 prefilterEnvMap(vec3 R, float roughness)
{
	vec3 N = R;
	vec3 V = R;
	vec3 color = vec3(0.0);
	float totalWeight = 0.0;
	float envMapDim = float(textureSize(cubemap, 0).s);
	for(uint i = 0u; i < pushConsts.numSamples; i++) {
		vec2 Xi = hammersley2d(i, pushConsts.numSamples);
		vec3 H = importanceSample_GGX(Xi, roughness, N);
		vec3 L = 2.0 * dot(V, H) * H - V;
		float dotNL = clamp(dot(N, L), 0.0, 1.0);
		if(dotNL > 0.0) {
            float dotNH = clamp(dot(N, H), 0.0, 1.0);
			float dotVH = clamp(dot(V, H), 0.0, 1.0);

			// Probability Distribution Function
			float pdf = DistributionGGX(N, H, roughness) * dotNH / (4.0 * dotVH) + 0.0001;
			// Solid angle of current sample
			float omegaS = 1.0 / (float(pushConsts.numSamples) * pdf);
			// Solid angle of 1 pixel across all cube faces
			float omegaP = 4.0 * PI / (6.0 * envMapDim * envMapDim);
			// Biased (+1.0) mip level for better result
			float mipLevel = roughness == 0.0 ? 0.0 : max(0.5 * log2(omegaS / omegaP) + 1.0, 0.0f);
			color += textureLod(cubemap, L, mipLevel).rgb * dotNL;
			totalWeight += dotNL;
		}
	}
	return (color / totalWeight);
}

void main()
{		
	vec3 N = normalize(direction);
	outColor = vec4(prefilterEnvMap(N, pushConsts.roughness), 1.0);
}