#include "constants.glsl"

vec3 cosineSampleHemisphere(const in vec2 randoms, inout float pdf) {
    float rx = 2.0 * randoms.x - 1;
    float ry = 2.0 * randoms.y - 1;

    vec3 dir = vec3(0, 0, 0);
    if (rx == 0 && ry == 0) {
        dir.x = 0;
        dir.z = 0;
    } 
    else if (abs(rx) > abs(ry))
    {
        float r = rx;
        float phi = PI_OVER_FOUR * (ry / rx);

        dir.x = r * cos(phi);
        dir.z = r * sin(phi);
    } 
    else 
    {
        float r = ry;
        float phi = PI_OVER_TWO - PI_OVER_FOUR * (rx / ry);

        dir.x = r * cos(phi);
        dir.z = r * sin(phi);
    }

    dir.y = sqrt(max(0.0, 1.0 - dir.x * dir.x - dir.z * dir.z));

    pdf = INV_PI * dir.y;
    pdf = max(pdf, EPSILON);

    return dir;
}

float cosineSampleHemispherePdf(float cosTheta)
{
    return cosTheta * INV_PI;
}

vec2 uniformSampleTriangle(vec2 rng)
{
    const float a = sqrt(1.0F - rng.x);
    return vec2(1 - a, a * rng.y);
}

vec2 concentricSampleDisk(const in vec2 randoms) {
    /* Map uniform random numbers to $[-1,1]^2$ */
    vec2 uOffset = 2.0 * randoms - vec2(1, 1);

    /* Handle degeneracy at the origin */
    if (uOffset.x == 0 && uOffset.y == 0) {
        return vec2(0, 0);
    }

    /* Apply concentric mapping to point */
    float theta, r;
    if (abs(uOffset.x) > abs(uOffset.y)) {
        r = uOffset.x;
        theta = PI_OVER_FOUR * (uOffset.y / uOffset.x);
    } else {
        r = uOffset.y;
        theta = PI_OVER_TWO - PI_OVER_FOUR * (uOffset.x / uOffset.y);
    }
    return r * vec2(cos(theta), sin(theta));
}
