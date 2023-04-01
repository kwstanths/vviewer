vec3 applyNormalMap(vec3 tangent, vec3 bitangent, vec3 normal, vec3 normalMap)
{
    mat3 TBN = mat3(tangent, bitangent, normal);
    vec3 N = normalMap;
    N = N * 2.0 - 1.0;
    N = normalize(TBN * N);

    return N;
}
