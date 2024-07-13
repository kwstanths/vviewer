/* Descriptor with the buffer with the instance data */
layout(set = 1, binding = 0) buffer readonly InstanceDataDescriptor {
    InstanceData data[16384];
} instances;