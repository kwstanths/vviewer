#include "VulkanMesh.hpp"

#include "Utils.hpp"

VulkanMesh::VulkanMesh(const Mesh & mesh) : Mesh(mesh)
{
}

VulkanMeshModel::VulkanMeshModel(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Mesh>& meshes)
{
    /* Create a VulkanMesh for every mesh, allocate gpu buffers for vertices and indices and push it back to the mesh vector */
    for (size_t i = 0; i < meshes.size(); i++) {

        Mesh& mesh = meshes[i];
        if (!mesh.hasNormals()) mesh.computeNormals();

        VulkanMesh * vkmesh = new VulkanMesh(mesh);
        createVertexBuffer(physicalDevice, device, transferQueue, transferCommandPool, mesh.getVertices(), vkmesh->m_vertexBuffer, vkmesh->m_vertexBufferMemory);
        createIndexBuffer(physicalDevice, device, transferQueue, transferCommandPool, mesh.getIndices(), vkmesh->m_indexBuffer, vkmesh->m_indexBufferMemory);

        m_meshes.push_back(vkmesh);
    }
    
}
