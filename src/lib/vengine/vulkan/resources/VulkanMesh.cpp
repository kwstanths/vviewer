#include "VulkanMesh.hpp"

#include "math/Constants.hpp"
#include "vulkan/common/VulkanUtils.hpp"
#include "vulkan/renderers/VulkanRendererPathTracing.hpp"

namespace vengine
{

VulkanMesh::VulkanMesh(const AssetInfo &info)
    : Mesh(info)
{
}

VulkanMesh::VulkanMesh(const Mesh &mesh, VulkanCommandInfo vci, bool generateBLAS)
    : Mesh(mesh)
{
    VkBufferUsageFlags rayTracingUsageFlags = VulkanRendererPathTracing::getBufferUsageFlags();

    createVertexBuffer(vci, m_vertices, rayTracingUsageFlags, m_vertexBuffer);
    createIndexBuffer(vci, m_indices, rayTracingUsageFlags, m_indexBuffer);

    if (generateBLAS) {
        m_blas.initializeBottomLevelAcceslerationStructure(vci, *this, glm::mat4(1.0F));
    }
}

void VulkanMesh::destroy(VkDevice device)
{
    m_vertexBuffer.destroy(device);
    m_indexBuffer.destroy(device);

    if (m_blas.initialized()) {
        m_blas.destroy(device);
    }
}

VulkanCube::VulkanCube(const AssetInfo &info, VulkanCommandInfo vci, bool generateBLAS)
    : VulkanMesh(info)
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

    m_nTriangles = m_indices.size() / 3U;

    createVertexBuffer(vci, m_vertices, {}, m_vertexBuffer);
    createIndexBuffer(vci, m_indices, {}, m_indexBuffer);

    if (generateBLAS) {
        m_blas.initializeBottomLevelAcceslerationStructure(vci, *this, glm::mat4(1.0F));
    }

    computeAABB();
}

}  // namespace vengine