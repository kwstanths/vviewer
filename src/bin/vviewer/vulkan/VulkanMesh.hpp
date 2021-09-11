#ifndef __VulkanMesh_hpp__
#define __VulkanMesh_hpp__

#include "IncludeVulkan.hpp"
#include "core/MeshModel.hpp"
#include "core/Mesh.hpp"

class VulkanMesh : public Mesh {
public:
    VulkanMesh(const Mesh& mesh);

    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferMemory;
    VkBuffer m_indexBuffer;
    VkDeviceMemory m_indexBufferMemory;
};

class VulkanMeshModel : public MeshModel {
public:
    VulkanMeshModel(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Mesh>& meshes, bool computeNormals = false);

private:

};

#endif