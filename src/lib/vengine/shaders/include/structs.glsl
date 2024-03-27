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
    vec4 exposure;      /* R = exposure, G = environment map intensity, B = lens radius , A = focal distance */
    vec4 background;    /* RGB = background color, A = environment type */
};

/* ModelData struct. A mirror of the CPU struct */
struct ModelData {
    mat4 model;
};

/* Material struct. A mirror of the CPU struct */
struct MaterialData
{
    vec4 albedo;                /* RGB: albedo, A: unused */
    vec4 metallicRoughnessAO;   /* R: metallic, G: roughness, B: AO, A = is transparent */
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

/* LightInstance struct. A mirror of the CPU struct */
struct LightInstance {
    uvec4 info;     /* R = LightData index, G = ModelData index, B = Object Description index if Mesh type, A = type (LightType) */
    vec4 position;  /* RGB = world position/direction, A = casts shadow or RGBA = row 0 of transform matrix if mesh type */
    vec4 position1; /* RGBA = row 1 of transform matrix if mesh type */
    vec4 position2; /* RGBA = row 2 of transform matrix if mesh type */
};

