#ifndef __VulkanAccelerationStructure_hpp__
#define __VulkanAccelerationStructure_hpp__

#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/common/VulkanStructs.hpp"
#include "vulkan/VulkanContext.hpp"
#include "VulkanBuffer.hpp"

namespace vengine
{

class VulkanMesh;

class VulkanAccelerationStructure
{
public:
    VulkanAccelerationStructure();

    VkResult initializeBottomLevelAcceslerationStructure(VulkanCommandInfo vci, const VulkanMesh &mesh, const glm::mat4 &transform);
    VkResult initializeTopLevelAcceslerationStructure(VulkanCommandInfo vci,
                                                      const std::vector<VkAccelerationStructureInstanceKHR> &instances);

    bool initialized() const { return m_initialzed; }

    void destroy(VkDevice device);

    const VkAccelerationStructureKHR &handle() const { return m_handle; }
    const VulkanBuffer &buffer() const { return m_buffer; }

private:
    bool m_initialzed = false;

    VkAccelerationStructureKHR m_handle = VK_NULL_HANDLE;
    VulkanBuffer m_buffer;
};

}  // namespace vengine

#endif