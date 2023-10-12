#include "VulkanMesh.hpp"

#include "math/Constants.hpp"
#include "vulkan/common/VulkanUtils.hpp"

namespace vengine
{

VulkanMesh::VulkanMesh(std::string name)
    : Mesh(name)
{
}

VulkanMesh::VulkanMesh(std::string name,
                       const std::vector<Vertex> &vertices,
                       const std::vector<uint32_t> &indices,
                       bool hasNormals,
                       bool hasUVs,
                       VkPhysicalDevice physicalDevice,
                       VkDevice device,
                       VkQueue transferQueue,
                       VkCommandPool transferCommandPool,
                       VkBufferUsageFlags extraUsageFlags)
    : Mesh(name, vertices, indices, hasNormals, hasUVs)
{
    createVertexBuffer(physicalDevice, device, transferQueue, transferCommandPool, m_vertices, extraUsageFlags, m_vertexBuffer);
    createIndexBuffer(physicalDevice, device, transferQueue, transferCommandPool, m_indices, extraUsageFlags, m_indexBuffer);
}

VulkanMesh::VulkanMesh(const Mesh &mesh,
                       VkPhysicalDevice physicalDevice,
                       VkDevice device,
                       VkQueue transferQueue,
                       VkCommandPool transferCommandPool,
                       VkBufferUsageFlags extraUsageFlags)
    : VulkanMesh(mesh.name(),
                 mesh.vertices(),
                 mesh.indices(),
                 mesh.hasNormals(),
                 mesh.hasUVs(),
                 physicalDevice,
                 device,
                 transferQueue,
                 transferCommandPool,
                 extraUsageFlags)
{
}

void VulkanMesh::destroy(VkDevice device)
{
    m_vertexBuffer.destroy(device);
    m_indexBuffer.destroy(device);
}

VulkanCube::VulkanCube(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool)
    : VulkanMesh("Cube")
{
    m_vertices = {{{-1.0, -1.0, 1.0}, glm::vec2(0), glm::vec3(0), glm::vec3(0), glm::vec3(0), glm::vec3(0)},
                  {{1.0, -1.0, 1.0}, glm::vec2(0), glm::vec3(0), glm::vec3(0), glm::vec3(0), glm::vec3(0)},
                  {{1.0, 1.0, 1.0}, glm::vec2(0), glm::vec3(0), glm::vec3(0), glm::vec3(0), glm::vec3(0)},
                  {{-1.0, 1.0, 1.0}, glm::vec2(0), glm::vec3(0), glm::vec3(0), glm::vec3(0), glm::vec3(0)},
                  {{-1.0, -1.0, -1.0}, glm::vec2(0), glm::vec3(0), glm::vec3(0), glm::vec3(0), glm::vec3(0)},
                  {{1.0, -1.0, -1.0}, glm::vec2(0), glm::vec3(0), glm::vec3(0), glm::vec3(0), glm::vec3(0)},
                  {{1.0, 1.0, -1.0}, glm::vec2(0), glm::vec3(0), glm::vec3(0), glm::vec3(0), glm::vec3(0)},
                  {{-1.0, 1.0, -1.0}, glm::vec2(0), glm::vec3(0), glm::vec3(0), glm::vec3(0), glm::vec3(0)}};

    m_indices = {0, 1, 2, 2, 3, 0, 1, 5, 6, 6, 2, 1, 7, 6, 5, 5, 4, 7, 4, 0, 3, 3, 7, 4, 4, 5, 1, 1, 0, 4, 3, 2, 6, 6, 7, 3};

    Mesh mesh("cube", m_vertices, m_indices, false, false);

    createVertexBuffer(physicalDevice, device, transferQueue, transferCommandPool, m_vertices, {}, m_vertexBuffer);
    createIndexBuffer(physicalDevice, device, transferQueue, transferCommandPool, m_indices, {}, m_indexBuffer);
}

}  // namespace vengine