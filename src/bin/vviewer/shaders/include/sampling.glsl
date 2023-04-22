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
		float phi = PI4 * (ry / rx);

		dir.x = r * cos(phi);
		dir.z = r * sin(phi);
	} 
	else 
	{
		float r = ry;
		float phi = PI2 - PI4 * (rx / ry);

		dir.x = r * cos(phi);
		dir.z = r * sin(phi);
	}

	dir.y = sqrt(max(0.0, 1.0 - dir.x * dir.x - dir.z * dir.z));

    pdf = INVPI * dir.y;
	pdf = max(pdf, 0.000001);

	return dir;
}
