#include "VulkanMesh.hpp"

#include "Utils.hpp"

VulkanMesh::VulkanMesh(const Mesh & mesh) : Mesh(mesh)
{
}

VulkanMeshModel::VulkanMeshModel(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Mesh>& meshes, bool computeNormals)
{
    for (size_t i = 0; i < meshes.size(); i++) {

        Mesh& mesh = meshes[i];
        if (computeNormals) mesh.computeNormals();

        VulkanMesh * vkmesh = new VulkanMesh(mesh);
        createVertexBuffer(physicalDevice, device, transferQueue, transferCommandPool, mesh.getVertices(), vkmesh->m_vertexBuffer, vkmesh->m_vertexBufferMemory);
        createIndexBuffer(physicalDevice, device, transferQueue, transferCommandPool, mesh.getIndices(), vkmesh->m_indexBuffer, vkmesh->m_indexBufferMemory);

        m_meshes.push_back(vkmesh);
    }
    
}
