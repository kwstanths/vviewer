#ifndef __VulkanBuffer_hpp__
#define __VulkanBuffer_hpp__

#include "vulkan/IncludeVulkan.hpp"

class VulkanBuffer  {
public:

    void destroy(VkDevice device);

    inline VkBuffer& vkbuffer() { return m_buffer; }
    inline VkBuffer getvkbuffer() const { return m_buffer; }

    inline VkDeviceMemory& vkmemory() { return m_memory; }
    inline VkDeviceMemory getvkmemory() const { return m_memory; }

private:
    VkBuffer m_buffer;
    VkDeviceMemory m_memory;
};

#endif

