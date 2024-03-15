/* Get the hit object geometry, its vertices and its indices buffers */
ObjDesc objResource = objDesc.i[gl_InstanceCustomIndexEXT];
Indices indices = Indices(objResource.indexAddress);
Vertices vertices = Vertices(objResource.vertexAddress);
MaterialData material = materialData.data[objResource.materialIndex];

/* Get hit triangle info */
ivec3  ind = indices.i[gl_PrimitiveID];
Vertex v0 = vertices.v[ind.x];
Vertex v1 = vertices.v[ind.y];
Vertex v2 = vertices.v[ind.z];

/* Calculate bayrcentric coordiantes */
const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

/* Compute coordinates of the hit position */
const vec3 localPosition      = v0.position * barycentricCoords.x + v1.position * barycentricCoords.y + v2.position * barycentricCoords.z;
const vec3 worldPosition = vec3(gl_ObjectToWorldEXT * vec4(localPosition, 1.0));

/* Compute normal, tangent and bitangent at hit position */
const vec3 localNormal		= v0.normal * barycentricCoords.x + v1.normal * barycentricCoords.y + v2.normal * barycentricCoords.z;
const vec3 localTangent 	= v0.tangent * barycentricCoords.x + v1.tangent * barycentricCoords.y + v2.tangent * barycentricCoords.z;
const vec3 localBitangent	= v0.bitangent * barycentricCoords.x + v1.bitangent * barycentricCoords.y + v2.bitangent * barycentricCoords.z;
vec3 worldNormal = normalize(vec3(localNormal * gl_WorldToObjectEXT));
vec3 worldTangent = normalize(vec3(localTangent * gl_WorldToObjectEXT));
vec3 worldBitangent = normalize(vec3(localBitangent * gl_WorldToObjectEXT));

/* Compute uvs */
const vec2 uvs = v0.uv * barycentricCoords.x + v1.uv * barycentricCoords.y + v2.uv * barycentricCoords.z;

/* Construct a geometry frame */
Frame frame = Frame(worldNormal, worldTangent, worldBitangent);
bool flipped = fixFrame(frame.normal, frame.tangent, frame.bitangent, gl_WorldRayDirectionEXT);