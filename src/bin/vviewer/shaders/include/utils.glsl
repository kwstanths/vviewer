float max3 (vec3 v) {
    return max (max (v.x, v.y), v.z);
}

vec3 getCameraPosition(mat4 invViewMatrix)
{
    return vec3(invViewMatrix[3][0], invViewMatrix[3][1], invViewMatrix[3][2]);
}

vec3 processNormalFromNormalMap(vec3 normalMapNormal)
{
    vec3 N = normalMapNormal * 2 - 1;
    return vec3(N.x, N.z, -N.y);
}

float distanceSquared(vec3 p0, vec3 p1)
{
    float distance = length(p0 - p1);
    return distance * distance;
}

bool isBlack(vec3 c)
{
	return c.x == 0 && c.y == 0 && c.z == 0;
}