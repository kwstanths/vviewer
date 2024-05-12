float HenyeyGreenstein(const in float cosTheta, const in float g) {
    float denom = 1.0 + g * g + 2.0 * g * cosTheta;
    return INV_4PI * (1.0 - g * g) / (denom * sqrt(denom));
}

float HG_p(const in vec3 wo, const in vec3 wi, const in float g) {
    return HenyeyGreenstein(dot(wo, wi), g);
}

float HG_Sample(const in vec3 wo, out vec3 wi, const in vec2 randoms, const in float g) {
    float cosTheta;
    if (abs(g) < 1e-3)
    {
        cosTheta = 1.0 - 2.0 * randoms.x;
    }
    else {
        float sqrTerm = (1.0 - g * g) / (1 + g - 2 * g * randoms.x);
        cosTheta = -(1 + g * g - sqrTerm * sqrTerm) / (2.0 * g);
    }

    float sinTheta = sqrt(max(0.0, 1 - cosTheta * cosTheta));
    float phi = 2.0 * PI * randoms.y;
    vec3 v1, v2;

    /* Create a coordinate system out of the incoming direction */
    createCoordinateSystem(wo, v1, v2);
    
    /* And trasform the generated sample along that incoming direction */
    wi = sinTheta * cos(phi) * v1 + sinTheta * sin(phi) * v2 + cosTheta * wo;

    return HenyeyGreenstein(cosTheta, g);
}