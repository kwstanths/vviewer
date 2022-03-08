
vec3 tonemapReinhard(vec3 color)
{
	return color / (color + vec3(1.0));
}

vec3 tonemapDefault1(vec3 color, float exposure)
{
	return vec3(1.0) - exp(-color * exposure);
}

vec3 tonemapDefault2(vec3 color, float exposure)
{
	return color * exp2(exposure);
}