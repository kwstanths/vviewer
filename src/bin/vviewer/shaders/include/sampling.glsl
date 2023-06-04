#include "rng.glsl"
#include "constants.glsl"

vec3 cosineSampleHemisphere(inout uint rngState, inout float pdf) {
	float rx = 2.0 * rand(rngState) - 1;
	float ry = 2.0 * rand(rngState) - 1;

	vec3 dir = vec3(0, 0, 0);
	if (rx == 0 && ry == 0) {
		dir.x = 0;
		dir.z = 0;
	} 
	else if (abs(rx) > abs(ry))
	{
		float r = rx;
		float phi = PI_OVER_FOUR * (ry / rx);

		dir.x = r * cos(phi);
		dir.z = r * sin(phi);
	} 
	else 
	{
		float r = ry;
		float phi = PI_OVER_TWO - PI_OVER_FOUR * (rx / ry);

		dir.x = r * cos(phi);
		dir.z = r * sin(phi);
	}

	dir.y = sqrt(max(0.0, 1.0 - dir.x * dir.x - dir.z * dir.z));

	pdf = INV_PI * dir.y;
	pdf = max(pdf, 0.000001);

	return dir;
}

float cosineSampleHemispherePdf(float cosTheta)
{
	return cosTheta * INV_PI;
}

vec2 uniformSampleTriangle(vec2 rng)
{
	const float a = sqrt(1.0F - rng.x);
	return vec2(1 - a, a * rng.y);
}

