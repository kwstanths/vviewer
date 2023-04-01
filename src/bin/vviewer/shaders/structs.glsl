/* Scene struct. A mirror of the CPU struct */
struct Scene {
    mat4 view;
	mat4 viewInverse;
    mat4 projection;
	mat4 projectionInverse;
    vec4 directionalLightDir;
    vec4 directionalLightColor;
    vec4 exposure;
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

