/* Scene struct. A mirror of the CPU struct */
struct Scene {
    mat4 view;
	mat4 viewInverse;
    mat4 projection;
	mat4 projectionInverse;
    vec4 directionalLightDir;
    vec4 directionalLightColor;
    vec4 exposure; /* R = exposure, G = Ambient environment map multiplier, B = , A = */
};

/* Material struct. A mirror of the CPU struct */
struct Material
{
    vec4 albedo; /* RGB: albedo, A: alpha */
    vec4 metallicRoughnessAOEmissive;  /* R = metallic, G = roughness, B = AO, A = emissive */
    vec4 uvTiling; /* R = u tiling, G = v tiling, B = unused, A = unused */
    uvec4 gTexturesIndices1;    /* R = albedo texture index, G = metallic texture index, B = roughness texture index, A = AO texture index */   
    uvec4 gTexturesIndices2;    /* R = emissive texture index, G = normal texture index, B = BRDF LUT texture index, A = unused */

    uvec4 padding;
    uvec4 padding1;
    uvec4 padding2;
};


#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
/* Struct for the object description of a mesh in the scene */
struct ObjDesc
{
    /* Pointers to GPU buffers */
	uint64_t vertexAddress;
	uint64_t indexAddress;
	uint materialIndex;
};

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

/* Struct with ray payload */
struct RayPayload {
    vec3 radiance;
    vec3 beta;

    vec3 origin;
    vec3 direction;
    bool stop;

    uint rngState;
};

/* Struct to represent lights */
/* Types of lights:
    0: point light
    1: directional light
    2: mesh light
    3: environment map
*/
struct Light {
    vec4 position;  /* RGB = world space position, A = light type  */
    vec4 direction; /* RGB = world space direction, A = mesh id */
    vec4 color;     /* RGB = color, A = mesh material id */
};

