#include "VulkanImage.hpp"

namespace vengine
{

VulkanImage::VulkanImage(){};

VulkanImage::VulkanImage(VkImageView view, VkFormat format)
    : m_view(view)
    , m_format(format){};

VulkanImage::VulkanImage(VkImage image, VkImageView view, VkDeviceMemory memory, VkFormat format)
    : m_image(image)
    , m_view(view)
    , m_memory(memory)
    , m_format(format)
{
}

void VulkanImage::destroy(VkDevice device)
{
    vkDestroyImage(device, m_image, nullptr);
    vkDestroyImageView(device, m_view, nullptr);
    vkFreeMemory(device, m_memory, nullptr);
}

}  // namespace vengine