float max3 (vec3 v) {
    return max (max (v.x, v.y), v.z);
}

vec3 getTranslation(mat4 m)
{
    return vec3(m[3][0], m[3][1], m[3][2]);
}

vec3 processNormalFromNormalMap(vec3 normalMapNormal)
{
    vec3 N = normalMapNormal * 2 - 1;
    return normalize(vec3(N.x, N.z, -N.y));
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

bool isBlack(vec3 c, float epsilon)
{
    return (abs(c.x) <=  epsilon) && (abs(c.y) <=  epsilon) && (abs(c.z) <=  epsilon);
}

float linearize_depth(float d,float zNear,float zFar)
{
    return zNear * zFar / (zFar + d * (zNear - zFar));
}

vec3 worldPositionFromDepth(float depth, vec2 screenUV, in mat4 projectionInverse, in mat4 viewInverse) 
{
    vec4 clipSpacePosition = vec4(screenUV * 2.0 - 1.0, depth, 1.0);
    vec4 viewSpacePosition = projectionInverse * clipSpacePosition;

    viewSpacePosition /= viewSpacePosition.w;

    vec4 worldSpacePosition = viewInverse * viewSpacePosition;

    return worldSpacePosition.xyz;
}