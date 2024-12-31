#include "VulkanModel3D.hpp"

#include "VulkanMesh.hpp"

namespace vengine
{

VulkanModel3D::VulkanModel3D(const AssetInfo &info)
    : Model3D(info)
{
}

VulkanModel3D::VulkanModel3D(const AssetInfo &info,
                             const Tree<ImportedModelNode> &importedNodeTree,
                             VulkanCommandInfo vci,
                             bool generateBLAS)
    : Model3D(info)
{
    importNode(importedNodeTree, m_nodeTree, vci, generateBLAS, {});
}

VulkanModel3D::VulkanModel3D(const AssetInfo &info,
                             const Tree<ImportedModelNode> &importedNodeTree,
                             const std::vector<Material *> &materials,
                             VulkanCommandInfo vci,
                             bool generateBLAS)
    : Model3D(info)
{
    importNode(importedNodeTree, m_nodeTree, vci, generateBLAS, materials);
}

void VulkanModel3D::destroy(VkDevice device)
{
    std::function<void(VkDevice, Tree<Model3DNode> &)> destroyR = [&](VkDevice device, Tree<Model3DNode> &node) {
        for (auto &m : node.data().meshes) {
            VulkanMesh *mesh = static_cast<VulkanMesh *>(m);
            mesh->destroy(device);
            delete mesh;
        }

        for (uint32_t i = 0; i < node.childrenCount(); i++) {
            destroyR(device, node.child(i));
        }
    };

    destroyR(device, m_nodeTree);
}

void VulkanModel3D::importNode(const Tree<ImportedModelNode> &importedNodeTree,
                               Tree<Model3DNode> &nodeTree,
                               VulkanCommandInfo vci,
                               bool generateBLAS,
                               const std::vector<Material *> &materials)
{
    Model3DNode model3DNode;
    model3DNode.name = importedNodeTree.data().name;
    for (uint32_t i = 0; i < importedNodeTree.data().meshes.size(); i++) {
        auto &mesh = importedNodeTree.data().meshes[i];
        auto *mat = (materials.size() > 0 ? materials[importedNodeTree.data().materialIndices[i]] : nullptr);

        auto vkmesh = new VulkanMesh(mesh, vci, generateBLAS);
        vkmesh->m_model = this;

        model3DNode.meshes.emplace_back(vkmesh);
        m_meshes[mesh.name()] = vkmesh;
        model3DNode.materials.emplace_back(mat);
    }
    model3DNode.transform = importedNodeTree.data().transform;

    nodeTree.data() = model3DNode;

    for (uint32_t i = 0; i < importedNodeTree.childrenCount(); i++) {
        importNode(importedNodeTree.child(i), m_nodeTree.add(), vci, generateBLAS, materials);
    }
}

}  // namespace vengine