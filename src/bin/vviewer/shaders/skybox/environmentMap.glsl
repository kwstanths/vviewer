const vec2 invAtan = vec2(0.1591, 0.3183);

vec2 sampleEquirectangularMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    uv.x = mod(uv.x + 0.25, 1.0);
    return uv;
}
