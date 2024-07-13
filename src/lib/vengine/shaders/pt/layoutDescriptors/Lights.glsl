/* Descriptor for lights and light instances */
layout(set = 5, binding = 0) uniform readonly LightDataUBO {
    LightData data[1024];
} lightData;

layout(set = 5, binding = 1) uniform readonly LightInstancesUBO {
    LightInstance data[1024];
} lightInstances;