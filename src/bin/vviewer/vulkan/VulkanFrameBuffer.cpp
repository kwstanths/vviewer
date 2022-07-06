#include "VulkanFramebuffer.hpp"

#include "VulkanUtils.hpp"

bool VulkanFrameBufferAttachment::init(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t width, uint32_t height, VkFormat format)
{
    m_format = format;

    bool ret = createImage(physicalDevice,
        device,
        width,
        height,
        1,
        format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_image,
        m_memory);

    if (!ret) {
        return ret;
    }

    m_view = createImageView(device, m_image, m_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    return true;
}

void VulkanFrameBufferAttachment::destroy(VkDevice device)
{
	vkDestroyImageView(device, m_view, nullptr);
	vkDestroyImage(device, m_image, nullptr);
	vkFreeMemory(device, m_memory, nullptr);
}

VkImageView VulkanFrameBufferAttachment::getView() const
{
    return m_view;
}
