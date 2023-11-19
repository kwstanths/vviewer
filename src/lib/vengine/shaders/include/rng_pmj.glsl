/*
    https://github.com/mmp/pbrt-v4/blob/a7d44bb121959067090c4e441b18d657558d7f34/src/pbrt/samplers.h#L221C16-L221C16
*/

#include "bluenoise.glsl"

vec2 getPMJ02BNSample(uint sequenceIndex, uint sampleIndex)
{
    sequenceIndex = sequenceIndex % PMJ_N_SEQUENCES;
    sampleIndex = sampleIndex % PMJ_N_SAMPLES;

    return vec2(pmjSequences.i[sequenceIndex][sampleIndex][0], pmjSequences.i[sequenceIndex][sampleIndex][1]);
}

uint64_t mixBits(uint64_t v) {
    v ^= (v >> 31);
    v *= 9202493588570546565UL;
    v ^= (v >> 27);
    v *= 9357036318526133325UL;
    v ^= (v >> 33);
    return v;
}

uint permutationElement(uint i, uint l, uint p) {
    uint w = l - 1;
    w |= w >> 1;
    w |= w >> 2;
    w |= w >> 4;
    w |= w >> 8;
    w |= w >> 16;
    do {
        i ^= p;
        i *= 0xe170893d;
        i ^= p >> 16;
        i ^= (i & w) >> 4;
        i ^= p >> 8;
        i *= 0x0929eb3f;
        i ^= p >> 23;
        i ^= (i & w) >> 1;
        i *= 1 | p >> 27;
        i *= 0x6935fa69;
        i ^= (i & w) >> 11;
        i *= 0x74dcb303;
        i ^= (i & w) >> 2;
        i *= 0x9e501cc3;
        i ^= (i & w) >> 2;
        i *= 0xc860a3df;
        i &= w;
        i ^= i >> 5;
    } while (i >= l);

    return (i + p) % l;
}

float rand1D(inout RayPayloadPrimary rayPayload)
{
    uint64_t hash = mixBits((uint64_t(rayPayload.pixel.x) << 48) ^ (uint64_t(rayPayload.pixel.y) << 32) ^
                                (uint64_t(rayPayload.dimension) << 16) ^ PMJ_SEED);

    uint index = permutationElement(rayPayload.sampleIndex, rayPayload.samplesPerPixel, uint(hash));

    float delta = getBlueNoise(rayPayload.dimension, rayPayload.pixel);

    rayPayload.dimension++;
    return min((index + delta) / rayPayload.samplesPerPixel, ONEMINUSEPSILON);
}

vec2 rand2D(inout RayPayloadPrimary rayPayload)
{
    uint index = rayPayload.sampleIndex;
    uint pmjInstance = rayPayload.dimension / 2;

    if (pmjInstance >= PMJ_N_SEQUENCES) {
        uint64_t hash =
            mixBits((uint64_t(rayPayload.pixel.x) << 48) ^ (uint64_t(rayPayload.pixel.y) << 32) ^
                    (uint64_t(rayPayload.dimension) << 16) ^ PMJ_SEED);
        index = permutationElement(rayPayload.sampleIndex, rayPayload.samplesPerPixel, uint(hash));
    }

    vec2 u = getPMJ02BNSample(pmjInstance, index);

    // u += vec2(getBlueNoise(dimension, pixel), getBlueNoise(dimension + 1, pixel));
    // if (u.x >= 1) {
    //     u.x -= 1;
    // }
    // if (u.y >= 1) {
    //     u.y -= 1;
    // }

    rayPayload.dimension += 2;
    return vec2(min(u.x, ONEMINUSEPSILON), min(u.y, ONEMINUSEPSILON));
}