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