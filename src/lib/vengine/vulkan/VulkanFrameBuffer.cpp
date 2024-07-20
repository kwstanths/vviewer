#include "VulkanFramebuffer.hpp"

#include "vulkan/common/VulkanInitializers.hpp"
#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/common/VulkanUtils.hpp"

namespace vengine
{

VkResult VulkanFrameBufferAttachment::init(const VulkanFrameBufferAttachmentInfo &info, const std::vector<VkImageView> &views)
{
    m_info = info;

    for (const VkImageView &view : views) {
        m_images.push_back(VulkanImage(view, info.format));
    }

    m_destroyData = false;

    return VK_SUCCESS;
}

VkResult VulkanFrameBufferAttachment::init(VulkanContext &context, const VulkanFrameBufferAttachmentInfo &info, uint32_t count)
{
    m_info = info;

    m_images.resize(count);

    VkImageCreateInfo imageInfo = vkinit::imageCreateInfo(
        {info.width, info.height, 1}, info.format, 1, info.numSamples, VK_IMAGE_TILING_OPTIMAL, info.usageFlags);

    for (uint32_t i = 0; i < count; i++) {
        createImage(context.physicalDevice(),
                    context.device(),
                    imageInfo,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    m_images[i].image(),
                    m_images[i].memory());

        VkImageViewCreateInfo imageViewInfo =
            vkinit::imageViewCreateInfo(m_images[i].image(), info.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        vkCreateImageView(context.device(), &imageViewInfo, nullptr, &m_images[i].view());

        m_images[i].format() = info.format;
    }

    m_destroyData = true;

    return VK_SUCCESS;
}

void VulkanFrameBufferAttachment::destroy(VkDevice device)
{
    if (m_destroyData) {
        for (VulkanImage &image : m_images) {
            image.destroy(device);
        }
    }
}

VkResult VulkanFrameBuffer::create(VulkanContext &context,
                                   uint32_t count,
                                   VkRenderPass renderPass,
                                   uint32_t width,
                                   uint32_t height,
                                   const std::vector<VulkanFrameBufferAttachment> &attachments)
{
    m_attachments = attachments;

    m_framebuffers.resize(count);
    for (uint32_t i = 0; i < count; i++) {
        /* Get the view for each attachment */
        std::vector<VkImageView> views;
        for (const VulkanFrameBufferAttachment &attachment : attachments) {
            views.push_back(attachment.image(i).view());
        }

        VkFramebufferCreateInfo framebufferInfo =
            vkinit::framebufferCreateInfo(renderPass, static_cast<uint32_t>(attachments.size()), views.data(), width, height);
        VULKAN_CHECK_CRITICAL(vkCreateFramebuffer(context.device(), &framebufferInfo, nullptr, &m_framebuffers[i]));
    }

    return VK_SUCCESS;
}

VkResult VulkanFrameBuffer::destroy(VkDevice device)
{
    for (auto &frameBuffer : m_framebuffers) {
        vkDestroyFramebuffer(device, frameBuffer, nullptr);
    }
    return VK_SUCCESS;
}

}  // namespace vengine