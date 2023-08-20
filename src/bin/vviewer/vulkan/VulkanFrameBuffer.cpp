#include "VulkanFramebuffer.hpp"

#include "vulkan/common/VulkanInitializers.hpp"
#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/common/VulkanUtils.hpp"

namespace vengine
{

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

    VkImageCreateInfo imageInfo =
        vkinit::imageCreateInfo({width, height, 1},
                                format,
                                1,
                                numSamples,
                                VK_IMAGE_TILING_OPTIMAL,
                                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | extraUsageFlags);
    createImage(physicalDevice, device, imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_image, m_memory);

    VkImageViewCreateInfo imageViewInfo = vkinit::imageViewCreateInfo(m_image, m_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    vkCreateImageView(device, &imageViewInfo, nullptr, &m_view);

    if (createResolveAttachment) {
        VkImageCreateInfo resolveImageInfo =
            vkinit::imageCreateInfo({width, height, 1},
                                    format,
                                    1,
                                    VK_SAMPLE_COUNT_1_BIT,
                                    VK_IMAGE_TILING_OPTIMAL,
                                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | extraUsageFlags);
        createImage(physicalDevice, device, resolveImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_imageResolve, m_memoryResolve);

        VkImageViewCreateInfo resolveImageViewInfo =
            vkinit::imageViewCreateInfo(m_imageResolve, m_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        vkCreateImageView(device, &resolveImageViewInfo, nullptr, &m_viewResolve);
    }

    return true;
}

void VulkanFrameBufferAttachment::destroy(VkDevice device)
{
    vkDestroyImageView(device, m_view, nullptr);
    vkDestroyImage(device, m_image, nullptr);
    vkFreeMemory(device, m_memory, nullptr);

    if (m_hasResolveAttachment) {
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

}  // namespace vengine