#ifndef __PMJSequences_hpp__
#define __PMJSequences_hpp__

#include <cstdint>

namespace vengine
{

/* Affects shaders/include/rng/rng_pmj_defines.glsl */
static constexpr uint32_t PMJ_SEQUENCES = 16;
static constexpr uint32_t PMJ_SAMPLES = 16384;
static constexpr uint32_t PMJ02BN_TABLE_SIZE_BYTES = PMJ_SEQUENCES * PMJ_SAMPLES * 2 * sizeof(float);

extern const float pmj02bnSequences[PMJ_SEQUENCES][PMJ_SAMPLES][2];

}  // namespace vengine

#endif