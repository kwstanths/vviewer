#include "VulkanModel3D.hpp"

#include "VulkanMesh.hpp"

namespace vengine
{

VulkanModel3D::VulkanModel3D(const std::string &name)
    : Model3D(name)
{
}

VulkanModel3D::VulkanModel3D(const std::string &name,
                             const Tree<ImportedModelNode> &importedData,
                             const std::vector<std::shared_ptr<Material>> &materials,
                             VkPhysicalDevice physicalDevice,
                             VkDevice device,
                             VkQueue queue,
                             VkCommandPool commandPool)
    : Model3D(name)
{
    importNode(importedData, m_data, physicalDevice, device, queue, commandPool, materials);
}

VulkanModel3D::VulkanModel3D(const std::string &name,
                             const Tree<ImportedModelNode> &importedData,
                             VkPhysicalDevice physicalDevice,
                             VkDevice device,
                             VkQueue queue,
                             VkCommandPool commandPool)
    : Model3D(name)
{
    importNode(importedData, m_data, physicalDevice, device, queue, commandPool, {});
}

void VulkanModel3D::destroy(VkDevice device)
{
    std::function<void(VkDevice, Tree<Model3DNode> &)> destroyR = [&](VkDevice device, Tree<Model3DNode> &node) {
        for (auto &m : node.data().meshes) {
            std::static_pointer_cast<VulkanMesh>(m)->destroy(device);
        }

        for (uint32_t i = 0; i < node.size(); i++) {
            destroyR(device, node.child(i));
        }
    };

    destroyR(device, m_data);
}

void VulkanModel3D::importNode(const Tree<ImportedModelNode> &node,
                               Tree<Model3DNode> &data,
                               VkPhysicalDevice physicalDevice,
                               VkDevice device,
                               VkQueue queue,
                               VkCommandPool commandPool,
                               const std::vector<std::shared_ptr<Material>> &materials)
{
    Model3DNode model3DNode;
    model3DNode.name = node.data().name;
    for (uint32_t i = 0; i < node.data().meshes.size(); i++) {
        auto &mesh = node.data().meshes[i];
        auto &mat = (materials.size() > 0 ? materials[node.data().materialIndices[i]] : nullptr);

        auto vkmesh = std::make_shared<VulkanMesh>(mesh, physicalDevice, device, queue, commandPool);
        vkmesh->m_model = this;

        model3DNode.meshes.emplace_back(vkmesh);
        m_meshes[mesh.name()] = vkmesh;
        model3DNode.materials.emplace_back(mat);
    }
    model3DNode.transform = node.data().transform;

    data.data() = model3DNode;

    for (uint32_t i = 0; i < node.size(); i++) {
        importNode(node.child(i), m_data.add(), physicalDevice, device, queue, commandPool, materials);
    }
}

}  // namespace vengine