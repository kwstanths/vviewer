float squareDistanceAttenuation(vec3 pos1, vec3 pos2)
{
    float distance    = length(pos1 - pos2);
    return 1.0 / (distance * distance);
}

float squareDistanceAttenuation(float d)
{
    return 1.0 / (d * d);
}