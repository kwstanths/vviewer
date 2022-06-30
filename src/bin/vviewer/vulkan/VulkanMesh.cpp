#include "VulkanMesh.hpp"

#include "VulkanUtils.hpp"
#include "math/Constants.hpp"

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

        vkmesh->m_meshModel = this;
        m_meshes.push_back(vkmesh);
    }
    
}

VulkanCube::VulkanCube(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool)
{
    std::vector<Vertex> vertices = {
        {{-1.0, -1.0,  1.0}, glm::vec2(0), glm::vec3(0), glm::vec3(0), glm::vec3(0), glm::vec3(0)},
        {{1.0, -1.0,  1.0}, glm::vec2(0), glm::vec3(0), glm::vec3(0), glm::vec3(0), glm::vec3(0)},
        {{ 1.0,  1.0,  1.0}, glm::vec2(0), glm::vec3(0), glm::vec3(0), glm::vec3(0), glm::vec3(0)},
        {{-1.0,  1.0,  1.0}, glm::vec2(0), glm::vec3(0), glm::vec3(0), glm::vec3(0), glm::vec3(0)},
        {{-1.0, -1.0, -1.0}, glm::vec2(0), glm::vec3(0), glm::vec3(0), glm::vec3(0), glm::vec3(0)},
        {{1.0, -1.0, -1.0}, glm::vec2(0), glm::vec3(0), glm::vec3(0), glm::vec3(0), glm::vec3(0)},
        {{1.0,  1.0, -1.0}, glm::vec2(0), glm::vec3(0), glm::vec3(0), glm::vec3(0), glm::vec3(0)},
        {{-1.0,  1.0, -1.0}, glm::vec2(0), glm::vec3(0), glm::vec3(0), glm::vec3(0), glm::vec3(0)}
    };

    std::vector<uint16_t> indices = {
        0, 1, 2,
        2, 3, 0,
        1, 5, 6,
        6, 2, 1,
        7, 6, 5,
        5, 4, 7,
        4, 0, 3,
        3, 7, 4,
        4, 5, 1,
        1, 0, 4,
        3, 2, 6,
        6, 7, 3
    };

    Mesh mesh(vertices, indices, false, false);
    VulkanMesh * vkmesh = new VulkanMesh(mesh);
    createVertexBuffer(physicalDevice, device, transferQueue, transferCommandPool, mesh.getVertices(), vkmesh->m_vertexBuffer, vkmesh->m_vertexBufferMemory);
    createIndexBuffer(physicalDevice, device, transferQueue, transferCommandPool, mesh.getIndices(), vkmesh->m_indexBuffer, vkmesh->m_indexBufferMemory);
     
    m_meshes.push_back(vkmesh);
}
