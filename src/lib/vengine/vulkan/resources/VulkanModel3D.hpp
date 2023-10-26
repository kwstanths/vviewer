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
                  VkCommandPool commandPool,
                  VkBufferUsageFlags extraUsageFlags = {});
    VulkanModel3D(const std::string &name,
                  const Tree<ImportedModelNode> &importedData,
                  const std::vector<std::shared_ptr<Material>> &materials,
                  VkPhysicalDevice physicalDevice,
                  VkDevice device,
                  VkQueue queue,
                  VkCommandPool commandPool,
                  VkBufferUsageFlags extraUsageFlags = {});

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
                    VkBufferUsageFlags extraUsageFlags,
                    const std::vector<std::shared_ptr<Material>> &materials);
};

}  // namespace vengine

#endif