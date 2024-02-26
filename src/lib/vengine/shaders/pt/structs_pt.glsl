#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
/* Struct for the object description of a mesh in the scene for PT */
struct ObjDesc
{
    /* Pointers to GPU buffers */
    uint64_t vertexAddress;
    uint64_t indexAddress;
    uint materialIndex;
    uint numTriangles;
};

/* Struct with ray payload for the primary ray in PT */
struct RayPayloadPrimary {
    vec3 radiance;          /* Ray radiance */
    vec3 beta;              /* Current throughput */
    uint depth;             /* Current depth */

    vec3 albedo;            /* First hit albedo */
    vec3 normal;            /* First hit normal */

    vec3 origin;            /* Ray origin */
    vec3 direction;         /* Ray direction */
    bool stop;              /* If true stop tracing */

#ifdef SAMPLING_RTGEMS
    uint rngState;          /* RNG state */
#endif 

#ifdef SAMPLING_PMJ
    uvec2 pixel;            /* Pixel indices */
    uint dimension;         /* Current sampling dimension */
    uint sampleIndex;       /* Current sampling index */
    uint samplesPerPixel;   /* Total samples per pixel */
#endif
};

/* Struct with ray payload for the secondary ray in PT */
struct RayPayloadSecondary {
    bool shadowed;
    float throughput;
};

/* Struct with ray payload for the NEE ray in PT */
struct RayPayloadNEE {
    vec3 emissive;
    float throughput;
    float pdf;    
};

/* Struct for PT light sampling */
struct LightSamplingRecord {
    vec3 direction;
    vec3 radiance;
    float pdf;
    bool isDeltaLight;
};