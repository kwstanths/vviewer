#ifndef __VulkanDataStructs_hpp__
#define __VulkanDataStructs_hpp__

#include <glm/glm.hpp>

struct CameraData {
    alignas(16) glm::mat4 m_view;
    alignas(16) glm::mat4 m_projection;
};

struct ModelData {
    alignas(16) glm::mat4 m_modelMatrix;
};

struct MaterialPBRData {
    alignas(16) glm::vec4 albedo;
    alignas(16) glm::vec4 metallicRoughnessAOEmissive;
};

#endif