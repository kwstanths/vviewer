#ifndef __VulkanMesh_hpp__
#define __VulkanMesh_hpp__

#include "IncludeVulkan.hpp"
#include "core/Mesh.hpp"

class VulkanMesh : public Mesh {
public:
    VulkanMesh(const Mesh& mesh);

    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferMemory;
    VkBuffer m_indexBuffer;
    VkDeviceMemory m_indexBufferMemory;
};

#endif