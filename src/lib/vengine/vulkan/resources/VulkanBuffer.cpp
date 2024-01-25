#include "VulkanBuffer.hpp"

namespace vengine
{

VulkanBuffer::VulkanBuffer(){};

VulkanBuffer::VulkanBuffer(VkBuffer buffer, VkDeviceMemory memory, VkDeviceOrHostAddressKHR address)
    : m_buffer(buffer)
    , m_memory(memory)
    , m_address(address)
{
}

void VulkanBuffer::destroy(VkDevice device)
{
    vkDestroyBuffer(device, m_buffer, nullptr);
    vkFreeMemory(device, m_memory, nullptr);
}

}  // namespace vengine