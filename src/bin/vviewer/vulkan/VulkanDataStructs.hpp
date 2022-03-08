#ifndef __VulkanDataStructs_hpp__
#define __VulkanDataStructs_hpp__

#include <glm/glm.hpp>

struct SceneData {
    glm::mat4 m_view;
    glm::mat4 m_projection;
    glm::vec4 m_directionalLightDir;
    glm::vec4 m_directionalLightColor;
    glm::vec3 m_exposure; /* R = exposure, G = , B = , A = */
};

struct ModelData {
    glm::mat4 m_modelMatrix;
};

struct MaterialPBRData {
    glm::vec4 albedo;
    glm::vec4 metallicRoughnessAOEmissive;
};

#endif