#include "rng.glsl"
#include "constants.glsl"

vec3 cosineSampleHemisphere(inout uint rngState, inout float pdf) {
	float r1 = rand(rngState);
	float r2 = rand(rngState);

	vec3 dir;
	float r = sqrt(r1);
	float phi = 2.0 * PI * r2;

	dir.x = r * cos(phi);
	dir.z = r * sin(phi);
	dir.y = sqrt(max(0.0, 1.0 - dir.x * dir.x - dir.z * dir.z));

    pdf = INVPI * dir.y;

	return normalize(dir);
}