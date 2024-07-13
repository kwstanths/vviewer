/* Descriptor with materials */
layout(set = 3, binding = 0) uniform readonly MaterialDataUBO
{
    MaterialData data[512];
} materialData;