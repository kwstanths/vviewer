#include "VulkanBuffer.hpp"

namespace vengine {

void VulkanBuffer::destroy(VkDevice device)
{
    vkDestroyBuffer(device, m_buffer, nullptr);
    vkFreeMemory(device, m_memory, nullptr);
}

}