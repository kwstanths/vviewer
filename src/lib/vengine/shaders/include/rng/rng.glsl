#if defined(SAMPLING_PMJ)
    #include "rng_pmj.glsl"
#elif defined(SAMPLING_RTGEMS)
    #include "rng_def.glsl"
#endif