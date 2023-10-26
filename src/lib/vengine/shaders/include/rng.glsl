
/* [Ray tracing gems II, chapter 14] */

float uintToFloat(uint x) {
	return uintBitsToFloat(0x3f800000 | (x >> 9)) - 1.f;
}

uint xorshift(inout uint rngState) {
	rngState ^= rngState << 13;
	rngState ^= rngState >> 17;
	rngState ^= rngState << 5;
	return rngState;
}

float rand1D(inout RayPayload rayPayload) {
	return uintToFloat(xorshift(rayPayload.rngState));
}

vec2 rand2D(inout RayPayload rayPayload) {
	return vec2(rand1D(rayPayload), rand1D(rayPayload)); 
}

uint jenkinsHash(uint x) {
	x += x << 10;
	x ^= x >> 6;
	x += x << 3;
	x ^= x >> 11;
	x += x << 15;
	return x;
}

uint initRNG(uvec2 pixel, uvec2 resolution, uint frame) {
	uint pixelIdx = pixel[0] * resolution.x + pixel[1];
	uint rngState = pixelIdx ^ jenkinsHash(frame);
	return jenkinsHash(rngState);
}