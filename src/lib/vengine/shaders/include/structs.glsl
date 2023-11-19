/* Struct for vertex data stored per vertex in geometry buffers */
struct Vertex
{
    vec3 position;
    vec2 uv;
    vec3 normal;
    vec3 color;
    vec3 tangent;
    vec3 bitangent;
};

/* SceneData struct. A mirror of the CPU struct */
struct SceneData {
    mat4 view;
	mat4 viewInverse;
    mat4 projection;
	mat4 projectionInverse;
    vec4 exposure; /* R = exposure, G = Ambient environment map multiplier, B = , A = */
};

/* ModelData struct. A mirror of the CPU struct */
struct ModelData {
    mat4 model;
};

/* Material struct. A mirror of the CPU struct */
struct MaterialData
{
    vec4 albedo;                /* RGB: albedo, A: unused */
    vec4 metallicRoughnessAO;   /* R: metallic, G: roughness, B: AO, A = unused */
    vec4 emissive;              /* RGB: emissive color, A = emissive intensity */
    uvec4 gTexturesIndices1;    /* R: albedo texture index, G: metallic texture index, B: roughness texture index, A: AO texture index */   
    uvec4 gTexturesIndices2;    /* R: emissive texture index, G: normal texture index, B: BRDF LUT texture index, A: unused */
    vec4 uvTiling;              /* R: u tiling, G: v tiling, B: unused, A: unused */
    
    uvec4 padding1;
    uvec4 padding2;
};

/* LightData struct. A mirror of the CPU struct */
struct LightData {
    vec4 color; /* RGB: color, A: intensity */
    uvec4 type;  /* R = type (LightType), GBA: unused */
    
    uvec4 padding1;
    uvec4 padding2;
};

/* LightComponent struct. A mirror of the CPU struct */
struct LightComponent {
    uvec4 info; /* R = LightData index, G = ModelData index, BA = unused */
    
    uvec4 padding1;
    uvec4 padding2;
    uvec4 padding3;
};

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

    vec3 origin;            /* Ray origin */
    vec3 direction;         /* Ray direction */
    bool stop;              /* If true stop tracing */

    uint rngState;          /* RNG state */
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

/* Struct to represent lights in PT */
struct Light {
    vec4 position;  /* RGB = world space position or column 1 of transform matrix, A = light type  */
    vec4 direction; /* RGB = world space direction or column 2 of transform matrix, A = mesh id */
    vec4 color;     /* RGB = color or column 3 of transform matrix, A = ... */
    vec4 transform; /* RGB = column 4 of transform matrix, A = ... */
};

/* Struct for PT light sampling */
struct LightSamplingRecord {
	vec3 direction;
	vec3 radiance;
	float pdf;
	bool isDeltaLight;
};
