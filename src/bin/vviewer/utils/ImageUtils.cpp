#include "ImageUtils.hpp"

namespace vengine {

float linearToSRGB(float linear) {
	float srgb;
	if (linear <= 0.0031308f) {
		srgb = linear * 12.92f;
	} else {
		srgb = 1.055f * std::pow(linear, 1.0f / 2.4f) - 0.055f;
	}
	return srgb * 255.f;
}

float sRGBToLinear(float srgb) {
	auto linear = srgb / 255.0f;
	if (linear <= 0.04045f) {
		linear = linear / 12.92f;
	} else {
		linear = std::pow((linear + 0.055f) / 1.055f, 2.4f);
	}
	return linear;
}

}