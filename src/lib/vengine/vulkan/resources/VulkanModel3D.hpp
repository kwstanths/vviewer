#ifndef __VulkanModel3D_hpp__
#define __VulkanModel3D_hpp__

#include "core/Model3D.hpp"
#include "core/io/AssimpLoadModel.hpp"
#include "vulkan/common/IncludeVulkan.hpp"

namespace vengine
{

class VulkanModel3D : public Model3D
{
public:
    VulkanModel3D(const std::string &name);
    VulkanModel3D(const std::string &name,
                  const Tree<ImportedModelNode> &importedData,
                  VkPhysicalDevice physicalDevice,
                  VkDevice device,
                  VkQueue queue,
                  VkCommandPool commandPool);
    VulkanModel3D(const std::string &name,
                  const Tree<ImportedModelNode> &importedData,
                  const std::vector<Material *> &materials,
                  VkPhysicalDevice physicalDevice,
                  VkDevice device,
                  VkQueue queue,
                  VkCommandPool commandPool);

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
                    VkPhysicalDevice physicalDevice,
                    VkDevice device,
                    VkQueue queue,
                    VkCommandPool commandPool,
                    const std::vector<Material *> &materials);
};

}  // namespace vengine

#endif