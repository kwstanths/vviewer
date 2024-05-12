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
    uint recursionDepth;    /* Recursion depth */
    uint surfaceDepth;      /* First surface hit depth */

    vec3 albedo;            /* First hit albedo */
    vec3 normal;            /* First hit normal */

    vec3 origin;                /* Ray origin */
    vec3 direction;             /* Ray direction */
    bool stop;                  /* If true stop tracing */
    bool insideVolume;          /* If true, ray traveled inside a volume */
    float vtmin;                /* The parametric t when we entered the volume, or when the ray started */
    uint volumeMaterialIndex;   /* The material index for the volume */

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

/* Struct with ray payload for the secondary (shadow) ray in PT */
struct RayPayloadSecondary {
    bool shadowed;              /* Ray is shadowed or not */
    vec3 throughput;            /* Ray throughput */
    bool insideVolume;          /* If true, ray traveled inside a volume */
    float vtmin;                /* The parametric t when we entered the volume, or when the ray started */
    uint volumeMaterialIndex;   /* The material index for the volume */
};

/* Struct with ray payload for the NEE (next event estimation) ray in PT */
struct RayPayloadNEE {
    vec3 emissive;              /* emissive value of ray */
    vec3 throughput;            /* Ray throughput */
    float pdf;                  /* pdf of sampling that emissive surface */
    bool insideVolume;          /* If true, ray traveled inside a volume */
    float vtmin;                /* The parametric t when we entered the volume, or when the ray started */
    uint volumeMaterialIndex;   /* The material index for the volume */
};

/* Struct for PT light sampling */
struct LightSamplingRecord {
    vec3 direction;
    vec3 radiance;
    float pdf;
    bool isDeltaLight;
};