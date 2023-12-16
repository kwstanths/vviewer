struct Frame {
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
};

bool fixFrame(inout vec3 normal, inout vec3 tangent, inout vec3 bitangent, vec3 ray)
{
    bool flipped = false;
    
    /* Flip frame if the ray comes from the other side */
    if (dot(normal, ray) > 0)
    {
        normal = -normal;
        tangent = -tangent;
        flipped = true;
    }

    normal = normalize(normal);
    tangent = normalize(tangent);
    bitangent = normalize(bitangent);

    /* Fix frame to be perpendicular */
    if (abs(dot(normal, tangent)) > 0.999)
    {
        /* If normal and tangent are parallel, construct a frame from the normal only */
        bitangent = abs(normal.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
        tangent = cross(bitangent, normal);
        bitangent = cross(normal, tangent);
    } else {
        bitangent = cross(normal, tangent);
        tangent = cross(bitangent, normal);
    }

    return flipped;
}

vec3 localToWorld(Frame frame, vec3 localV)
{
    return frame.tangent * localV.x + frame.normal * localV.y + frame.bitangent * localV.z;
}

vec3 worldToLocal(Frame frame, vec3 worldV)
{
    return vec3(dot(worldV, frame.tangent), dot(worldV, frame.normal), dot(worldV, frame.bitangent));
}

void applyNormalToFrame(inout Frame frame, vec3 newNormal)
{
    vec3 newNormal_world = localToWorld(frame, newNormal);

    frame.normal = newNormal_world;
    frame.tangent = cross(frame.normal, frame.bitangent);
    frame.bitangent = cross(frame.tangent, frame.normal);
}