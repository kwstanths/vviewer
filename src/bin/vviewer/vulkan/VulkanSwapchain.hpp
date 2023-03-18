#ifndef __VulkanSwapchain_hpp__
#define __VulkanSwapchain_hpp__

#include "IncludeVulkan.hpp"
#include "VulkanContext.hpp"

/* Initializes a swapchain and a depth buffer */
class VulkanSwapchain {
public:
    VulkanSwapchain(VulkanContext& vkctx);
    ~VulkanSwapchain();

    bool init(uint32_t width, uint32_t height);
    void destroy();
    bool isInitialized() const { return m_initialized; };

    VkSwapchainKHR swapchain() { return m_swapchain; }

    uint32_t imageCount() { return static_cast<uint32_t>(m_swapchainImageViews.size()); }
    VkExtent2D& extent() { return m_extent; }
    VkFormat& format() { return m_format; }
    VkFormat& depthFormat() { return m_depthFormat; }

    VkImageView& swapchainImageView(uint32_t i) { return m_swapchainImageViews[i]; }
    VkImageView& msaaImageView() { return m_msaaImageView; }
    VkImageView& depthStencilImageView() { return m_depthImageView; }

private:
    VulkanContext& m_vkctx;

    /* Swapchain data */
    bool m_initialized = false;
    VkSwapchainKHR m_swapchain;
    VkFormat m_format;
    VkExtent2D m_extent;

    /* Swapchain images */
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;

    /* MSAA target images */
    VkImage m_msaaImage;
    VkDeviceMemory m_msaaImageMemory;
    VkImageView m_msaaImageView;

    /* Depth stencil buffer */
    VkImage m_depthImage;
    VkDeviceMemory m_depthImageMemory;
    VkImageView m_depthImageView;
    VkFormat m_depthFormat;

    void createImageViews();
    void createDepthBuffer();

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height);
    VkFormat findDepthFormat();
};

#endif
