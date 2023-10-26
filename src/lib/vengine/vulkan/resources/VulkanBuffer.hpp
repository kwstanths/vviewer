#ifndef __VulkanBuffer_hpp__
#define __VulkanBuffer_hpp__

#include "vulkan/common/IncludeVulkan.hpp"

namespace vengine
{

class VulkanBuffer
{
public:
    void destroy(VkDevice device);

    inline VkBuffer &buffer() { return m_buffer; }
    inline const VkBuffer &buffer() const { return m_buffer; }

    inline VkDeviceMemory &memory() { return m_memory; }
    inline const VkDeviceMemory &memory() const { return m_memory; }

private:
    VkBuffer m_buffer;
    VkDeviceMemory m_memory;
};

}  // namespace vengine

#endif
