#ifndef __VulkanMesh_hpp__
#define __VulkanMesh_hpp__

#include <array>

#include "core/MeshModel.hpp"
#include "core/Mesh.hpp"

#include "vulkan/IncludeVulkan.hpp"
#include "VulkanBuffer.hpp"

class VulkanVertex {
public:
    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptionsFull()
    {
        std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, uv);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, normal);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(Vertex, tangent);

        attributeDescriptions[4].binding = 0;
        attributeDescriptions[4].location = 4;
        attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[4].offset = offsetof(Vertex, bitangent);

        return attributeDescriptions;
    }

    static std::array<VkVertexInputAttributeDescription, 1> getAttributeDescriptionsPos()
    {
        std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, position);

        return attributeDescriptions;
    }
};

class VulkanMesh : public Mesh {
public:
    VulkanMesh(const Mesh& mesh);

    void destroy(VkDevice device);

    inline VulkanBuffer& vertexBuffer() { return m_vertexBuffer; }
    inline VulkanBuffer getVertexBuffer() const { return m_vertexBuffer; }

    inline VulkanBuffer& indexBuffer() { return m_indexBuffer; }
    inline VulkanBuffer getIndexBuffer() const { return m_indexBuffer; }

    inline VkIndexType indexType() const { return VK_INDEX_TYPE_UINT32; }

private:
    VulkanBuffer m_vertexBuffer;
    VulkanBuffer m_indexBuffer;
};

class VulkanMeshModel : public MeshModel {
public:
    VulkanMeshModel(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Mesh>& meshes, VkBufferUsageFlags extraUsageFlags);
    
    void destroy(VkDevice device);

private:

};

/* ---------- Some shapes ---------- */

class VulkanCube : public MeshModel {
public:
    VulkanCube(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool);
private:

};


#endif