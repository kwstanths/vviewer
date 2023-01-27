float max3 (vec3 v) {
    return max (max (v.x, v.y), v.z);
}

vec3 getCameraPosition(mat4 invViewMatrix)
{
    return vec3(invViewMatrix[3][0], invViewMatrix[3][1], invViewMatrix[3][2]);
}