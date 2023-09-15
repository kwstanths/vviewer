#ifndef __BlueNoise_hpp__
#define __BlueNoise_hpp__

#include <cstdint>

namespace vengine
{

/* Affects shaders/include/bluenoise.glsl */
static constexpr uint32_t BLUE_NOISE_TEXTURES = 48;
static constexpr uint32_t BLUE_NOISE_RESOLUTION = 128;
static constexpr uint32_t BLUE_NOISE_TABLE_SIZE_BYTES =
    BLUE_NOISE_TEXTURES * BLUE_NOISE_RESOLUTION * BLUE_NOISE_RESOLUTION * sizeof(float);

extern const float bluNoiseTextures[BLUE_NOISE_TEXTURES][BLUE_NOISE_RESOLUTION][BLUE_NOISE_RESOLUTION];

}  // namespace vengine

#endif