#include "VulkanFramebuffer.hpp"

#include "VulkanUtils.hpp"

bool VulkanFrameBufferAttachment::init(VkPhysicalDevice physicalDevice, 
    VkDevice device, 
    uint32_t width, 
    uint32_t height, 
    VkFormat format, 
    VkSampleCountFlagBits numSamples,
    bool createResolveAttachment,
    VkImageUsageFlags extraUsageFlags)
{
    m_format = format;
    m_hasResolveAttachment = createResolveAttachment;

    bool ret = createImage(physicalDevice,
        device,
        width,
        height,
        1,
        numSamples,
        format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | extraUsageFlags,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_image,
        m_memory);

    if (!ret) {
        return ret;
    }

    m_view = createImageView(device, m_image, m_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    if (createResolveAttachment)
    {
        ret = createImage(physicalDevice, 
            device, 
            width, 
            height,
            1, 
            VK_SAMPLE_COUNT_1_BIT, 
            format, 
            VK_IMAGE_TILING_OPTIMAL, 
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | extraUsageFlags, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
            m_imageResolve, 
            m_memoryResolve);
            
        m_viewResolve = createImageView(device, m_imageResolve, m_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }

    return true;
}

void VulkanFrameBufferAttachment::destroy(VkDevice device)
{
	vkDestroyImageView(device, m_view, nullptr);
	vkDestroyImage(device, m_image, nullptr);
	vkFreeMemory(device, m_memory, nullptr);

    if (m_hasResolveAttachment)
    {
        vkDestroyImageView(device, m_viewResolve, nullptr);
	    vkDestroyImage(device, m_imageResolve, nullptr);
	    vkFreeMemory(device, m_memoryResolve, nullptr);
    }
}

VkImageView VulkanFrameBufferAttachment::getView() const
{
    return m_view;
}

VkImage VulkanFrameBufferAttachment::getImage() const
{
    return m_image;
}

bool VulkanFrameBufferAttachment::hasResolveTargets() const
{
    return m_hasResolveAttachment;
}

VkImageView VulkanFrameBufferAttachment::getViewResolve() const
{
    return m_viewResolve;
}

VkImage VulkanFrameBufferAttachment::getImageResolve() const
{
    return m_imageResolve;
}
