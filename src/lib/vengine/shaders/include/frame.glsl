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

/*
    Sets newNormal to be the new frame normal, newNormal must be expressed in the frame coordinate system
*/
void applyNormalToFrame(inout Frame frame, vec3 newNormal)
{
    vec3 newNormal_world = localToWorld(frame, newNormal);

    frame.normal = newNormal_world;
    frame.tangent = cross(frame.normal, frame.bitangent);
    frame.bitangent = cross(frame.tangent, frame.normal);
}

/*
    Creates a coordinate system out of an input vector
*/
void createCoordinateSystem(const in vec3 v1, out vec3 v2, out vec3 v3)
{
    if (abs(v1.x) > abs(v1.y))
    {
        v2 = vec3(-v1.z, 0, v1.x) / sqrt(v1.x * v1.x + v1.z * v1.z);
    }
    else 
    {
        v2 = vec3(0, v1.z, -v1.y) / sqrt(v1.y * v1.y + v1.z * v1.z);
    }
    v3 = cross(v1, v2);
}

/*
    Create a Frame from a normal
*/
Frame frameFromNormal(in vec3 normal)
{
    vec3 v2, v3;
    createCoordinateSystem(normal, v2, v3);
    
    Frame frame;
    frame.normal = normal;
    frame.tangent = v2;
    frame.bitangent = v3;
    return frame;
}