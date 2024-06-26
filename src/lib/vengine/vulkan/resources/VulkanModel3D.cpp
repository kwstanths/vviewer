#include "VulkanModel3D.hpp"

#include "VulkanMesh.hpp"

namespace vengine
{

VulkanModel3D::VulkanModel3D(const AssetInfo &info)
    : Model3D(info)
{
}

VulkanModel3D::VulkanModel3D(const AssetInfo &info,
                             const Tree<ImportedModelNode> &importedData,
                             VulkanCommandInfo vci,
                             bool generateBLAS)
    : Model3D(info)
{
    importNode(importedData, m_data, vci, generateBLAS, {});
}

VulkanModel3D::VulkanModel3D(const AssetInfo &info,
                             const Tree<ImportedModelNode> &importedData,
                             const std::vector<Material *> &materials,
                             VulkanCommandInfo vci,
                             bool generateBLAS)
    : Model3D(info)
{
    importNode(importedData, m_data, vci, generateBLAS, materials);
}

void VulkanModel3D::destroy(VkDevice device)
{
    std::function<void(VkDevice, Tree<Model3DNode> &)> destroyR = [&](VkDevice device, Tree<Model3DNode> &node) {
        for (auto &m : node.data().meshes) {
            VulkanMesh *mesh = static_cast<VulkanMesh *>(m);
            mesh->destroy(device);
            delete mesh;
        }

        for (uint32_t i = 0; i < node.size(); i++) {
            destroyR(device, node.child(i));
        }
    };

    destroyR(device, m_data);
}

void VulkanModel3D::importNode(const Tree<ImportedModelNode> &node,
                               Tree<Model3DNode> &data,
                               VulkanCommandInfo vci,
                               bool generateBLAS,
                               const std::vector<Material *> &materials)
{
    Model3DNode model3DNode;
    model3DNode.name = node.data().name;
    for (uint32_t i = 0; i < node.data().meshes.size(); i++) {
        auto &mesh = node.data().meshes[i];
        auto *mat = (materials.size() > 0 ? materials[node.data().materialIndices[i]] : nullptr);

        auto vkmesh = new VulkanMesh(mesh, vci, generateBLAS);
        vkmesh->m_model = this;

        model3DNode.meshes.emplace_back(vkmesh);
        m_meshes[mesh.name()] = vkmesh;
        model3DNode.materials.emplace_back(mat);
    }
    model3DNode.transform = node.data().transform;

    data.data() = model3DNode;

    for (uint32_t i = 0; i < node.size(); i++) {
        importNode(node.child(i), m_data.add(), vci, generateBLAS, materials);
    }
}

}  // namespace vengine