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

/* Construct a geometry frame */
Frame frame = Frame(worldNormal, worldTangent, worldBitangent);
bool flipped = fixFrame(frame.normal, frame.tangent, frame.bitangent, gl_WorldRayDirectionEXT);