#ifndef __VulkanDataStructs_hpp__
#define __VulkanDataStructs_hpp__

#include <glm/glm.hpp>

struct ModelData {
    glm::mat4 m_modelMatrix;
};

struct MaterialData {
    glm::vec4 albedo; /* RGB: albedo, A:alpha (unused) */
    glm::vec4 metallicRoughnessAOEmissive;  /* R = metallic, G = roughness, B = AO, A = emissive */
};

struct PushBlockForwardPass {
    glm::vec4 selected; /* R = object is selected, R = , B = , A = */
};

#endif