#ifndef __VulkanDataStructs_hpp__
#define __VulkanDataStructs_hpp__

#include <glm/glm.hpp>

struct ModelData {
    glm::mat4 m_modelMatrix;
};

struct MaterialPBRData {
    glm::vec4 albedo;
    glm::vec4 metallicRoughnessAOEmissive;
};

#endif