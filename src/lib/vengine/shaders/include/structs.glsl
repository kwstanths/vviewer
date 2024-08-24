#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

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
    vec4 volumes;      /* R = material id of camera volume, G = near plane, B = far plane, A = unused */
};

/* InstanceData struct. A mirror of the CPU struct */
struct InstanceData {
    mat4 model;
    vec4 id;
    uint materialIndex;
    uint64_t vertexAddress;
    uint64_t indexAddress;
    uint numTriangles;
    
    uint padding1;
    uint padding2;
    uint padding3;
    uint padding4;
};

/* Material struct. A mirror of the CPU struct */
struct MaterialData
{
    vec4 albedo;                /* RGB: albedo, A: alpha */
    vec4 metallicRoughnessAO;   /* R: metallic, G: roughness, B: AO, A = is transparent */
    vec4 emissive;              /* RGB: emissive color, A = emissive intensity */
    uvec4 gTexturesIndices1;    /* R: albedo texture index, G: metallic texture index, B: roughness texture index, A: AO texture index */   
    uvec4 gTexturesIndices2;    /* R: emissive texture index, G: normal texture index, B: BRDF LUT texture index, A: unused */
    vec4 uvTiling;              /* R: u tiling, G: v tiling, B: material type, A: unused */
    
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
    uvec4 info;     /* R = LightData index, G = InstanceData index, B = Object Description index if Mesh type, A = type (LightType) */
    vec4 position;  /* RGB = world position/direction, A = casts shadow or RGBA = row 0 of transform matrix if mesh type */
    vec4 position1; /* RGBA = row 1 of transform matrix if mesh type */
    vec4 position2; /* RGBA = row 2 of transform matrix if mesh type */
};

