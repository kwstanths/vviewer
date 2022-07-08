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
    glm::vec4 selected; /* RGB = ID of object, A = if object is selected */
};

struct PushBlockForward3DUI {
    glm::mat4 modelMatrix;
    glm::vec4 color;
    glm::vec4 selected;
};

#endif