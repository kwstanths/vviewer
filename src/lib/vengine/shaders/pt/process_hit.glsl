/* Get the hit object geometry, its vertices and its indices buffers */
InstanceData instanceData = instances.data[gl_InstanceCustomIndexEXT];
Indices indices = Indices(instanceData.indexAddress);
Vertices vertices = Vertices(instanceData.vertexAddress);
MaterialData material = materialData.data[instanceData.materialIndex];

/* Get hit triangle info */
ivec3  ind = indices.i[gl_PrimitiveID];
Vertex v0 = vertices.v[ind.x];
Vertex v1 = vertices.v[ind.y];
Vertex v2 = vertices.v[ind.z];

/* Calculate bayrcentric coordiantes */
const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

/* Compute uvs */
const vec2 uvs = v0.uv * barycentricCoords.x + v1.uv * barycentricCoords.y + v2.uv * barycentricCoords.z;