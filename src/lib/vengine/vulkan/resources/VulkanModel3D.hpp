#ifndef __VulkanModel3D_hpp__
#define __VulkanModel3D_hpp__

#include "core/Model3D.hpp"
#include "core/io/AssimpLoadModel.hpp"
#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/common/VulkanStructs.hpp"

namespace vengine
{

class VulkanModel3D : public Model3D
{
public:
    VulkanModel3D(const AssetInfo &info);
    VulkanModel3D(const AssetInfo &info, const Tree<ImportedModelNode> &importedData, VulkanCommandInfo vci, bool generateBLAS);
    VulkanModel3D(const AssetInfo &info,
                  const Tree<ImportedModelNode> &importedData,
                  const std::vector<Material *> &materials,
                  VulkanCommandInfo vci,
                  bool generateBLAS);

    void destroy(VkDevice device);

private:
    /**
     * @brief Create a model3d node from an imported node
     *
     * @param importedNode
     * @param data
     * @param physicalDevice
     * @param device
     * @param queue
     * @param commandPool
     * @param extraUsageFlags
     */
    void importNode(const Tree<ImportedModelNode> &importedNode,
                    Tree<Model3DNode> &data,
                    VulkanCommandInfo vci,
                    bool generateBLAS,
                    const std::vector<Material *> &materials);
};

}  // namespace vengine

#endif