#ifndef __VulkanSwapchain_hpp__
#define __VulkanSwapchain_hpp__

#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/VulkanContext.hpp"
#include "vulkan/resources/VulkanImage.hpp"
#include "vulkan/VulkanFramebuffer.hpp"

namespace vengine
{

/* Initializes a swapchain and a depth buffer */
class VulkanSwapchain
{
public:
    VulkanSwapchain(VulkanContext &vkctx);
    ~VulkanSwapchain();

    VkResult initResources(uint32_t width, uint32_t height);
    void releaseResources();
    bool isInitialized() const { return m_initialized; };

    VkSwapchainKHR swapchain() { return m_swapchain; }

    uint32_t imageCount() { return static_cast<uint32_t>(m_swapchainImageViews.size()); }
    VkExtent2D &extent() { return m_extent; }
    VkFormat &format() { return m_format; }
    VkFormat &depthFormat() { return m_depthFormat; }

    VkImageView &swapchainImageView(uint32_t i) { return m_swapchainImageViews[i]; }
    VkImageView &depthImageView(uint32_t i) { return m_depthImages[i].view(); }
    VkImageView &stencilImageView(uint32_t i) { return m_stencilImageViews[i]; }

    const VulkanFrameBufferAttachment &swapchainAttachment() { return m_framebufferAttachmentSwapchain; }
    const VulkanFrameBufferAttachment &depthAttachment() { return m_framebufferAttachmentDepth; }
    const VulkanFrameBufferAttachment &stencilAttachment() { return m_framebufferAttachmentStencil; }

private:
    VulkanContext &m_vkctx;

    /* Swapchain data */
    bool m_initialized = false;
    VkSwapchainKHR m_swapchain;
    VkFormat m_format;
    VkExtent2D m_extent;

    /* Swapchain images */
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;
    /* Frame buffer attachments for said images */
    VulkanFrameBufferAttachment m_framebufferAttachmentSwapchain;
    VulkanFrameBufferAttachment m_framebufferAttachmentDepth;
    VulkanFrameBufferAttachment m_framebufferAttachmentStencil;

    /* Depth stencil buffer */
    std::vector<VulkanImage> m_depthImages;
    std::vector<VkImageView> m_stencilImageViews;
    VkFormat m_depthFormat;

    VkResult createImageViews();
    VkResult createDepthBuffer();
    VkResult createFrameBufferAttachments();

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, uint32_t width, uint32_t height);
    VkFormat findDepthFormat();
};

}  // namespace vengine

#endif
