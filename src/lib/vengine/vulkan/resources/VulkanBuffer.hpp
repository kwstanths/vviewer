#ifndef __VulkanBuffer_hpp__
#define __VulkanBuffer_hpp__

#include "vulkan/common/IncludeVulkan.hpp"

namespace vengine
{

/**
 * @brief Represents a VkBuffer and its memory
 *
 */
class VulkanBuffer
{
public:
    VulkanBuffer();
    VulkanBuffer(VkBuffer buffer, VkDeviceMemory memory, VkDeviceOrHostAddressKHR address);

    void destroy(VkDevice device);

    inline VkBuffer &buffer() { return m_buffer; }
    inline const VkBuffer &buffer() const { return m_buffer; }

    inline VkDeviceMemory &memory() { return m_memory; }
    inline const VkDeviceMemory &memory() const { return m_memory; }

    inline VkDeviceOrHostAddressKHR &address() { return m_address; }
    inline const VkDeviceOrHostAddressKHR &address() const { return m_address; }

private:
    VkBuffer m_buffer = {};
    VkDeviceMemory m_memory = {};
    VkDeviceOrHostAddressKHR m_address = {};
};

}  // namespace vengine

#endif
