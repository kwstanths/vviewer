#ifndef __VulkanFramebuffer_hpp__
#define __VulkanFramebuffer_hpp__

#include "vulkan/VulkanContext.hpp"
#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/resources/VulkanImage.hpp"

namespace vengine
{

/**
 * @brief  Frame buffer attachment description
 *
 */
struct VulkanFrameBufferAttachmentInfo {
    std::string name;
    uint32_t width;
    uint32_t height;
    VkFormat format;
    VkSampleCountFlagBits numSamples;
    VkImageUsageFlags usageFlags;

    VulkanFrameBufferAttachmentInfo(){};
    VulkanFrameBufferAttachmentInfo(std::string n, uint32_t w, uint32_t h, VkFormat f, VkSampleCountFlagBits s)
        : name(n)
        , width(w)
        , height(h)
        , format(f)
        , numSamples(s)
    {
        usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    VulkanFrameBufferAttachmentInfo(std::string n,
                                    uint32_t w,
                                    uint32_t h,
                                    VkFormat f,
                                    VkSampleCountFlagBits s,
                                    VkImageUsageFlags extraFlags)
        : name(n)
        , width(w)
        , height(h)
        , format(f)
        , numSamples(s)
    {
        usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | extraFlags;
    }
};

/**
 * @brief i.e. A render target
 *
 */
class VulkanFrameBufferAttachment
{
public:
    /**
     * @brief Create an attachment from existing image view
     *
     * @param info
     * @param view
     * @return VkResult
     */
    VkResult init(const VulkanFrameBufferAttachmentInfo &info, const std::vector<VkImageView> &views);

    /**
     * @brief Create a new attachment
     *
     * @param context
     * @param info
     * @return VkResult
     */
    VkResult init(VulkanContext &context, const VulkanFrameBufferAttachmentInfo &info, uint32_t count);

    void destroy(VkDevice device);

    const VulkanFrameBufferAttachmentInfo &info() { return m_info; }

    const VulkanImage &image(uint32_t i) const { return m_images[i]; }

private:
    VulkanFrameBufferAttachmentInfo m_info;

    bool m_destroyData = false;
    std::vector<VulkanImage> m_images;
};

class VulkanFrameBuffer
{
public:
    VkResult create(VulkanContext &context,
                    uint32_t count,
                    VkRenderPass renderPass,
                    uint32_t width,
                    uint32_t height,
                    const std::vector<VulkanFrameBufferAttachment> &attachments);

    VkResult destroy(VkDevice device);

    const VkFramebuffer &framebuffer(uint32_t i) const { return m_framebuffers[i]; }
    VkFramebuffer &framebuffer(uint32_t i) { return m_framebuffers[i]; }

protected:
private:
    std::vector<VkFramebuffer> m_framebuffers;

    std::vector<VulkanFrameBufferAttachment> m_attachments;
};

}  // namespace vengine

#endif