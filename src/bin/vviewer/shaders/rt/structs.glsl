#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

/* Struct for the object description of a mesh in the scene */
struct ObjDesc
{
    /* Pointers to GPU buffers */
	uint64_t vertexAddress;
	uint64_t indexAddress;
	int materialIndex;
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

/* Material struct */
struct Material 
{
	vec4 albedo;
};