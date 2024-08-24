#include "VulkanSwapchain.hpp"

#include <algorithm>

#include "vulkan/common/VulkanInitializers.hpp"
#include "vulkan/common/VulkanUtils.hpp"

namespace vengine
{

VulkanSwapchain::VulkanSwapchain(VulkanContext &vkctx)
    : m_vkctx(vkctx)
{
}

VulkanSwapchain::~VulkanSwapchain()
{
}

VkResult VulkanSwapchain::initResources(uint32_t width, uint32_t height)
{
    VulkanSwapChainDetails details = querySwapChainSupport(m_vkctx.physicalDevice(), m_vkctx.surface());

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(details.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(details.presentModes);
    VkExtent2D extent = chooseSwapExtent(details.capabilities, width, height);

    uint32_t imageCount = details.capabilities.minImageCount + 1;
    if (details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount) {
        imageCount = details.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_vkctx.surface();
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VulkanQueueFamilyIndices &indices = m_vkctx.queueFamilyIndices();
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;      // Optional
        createInfo.pQueueFamilyIndices = nullptr;  // Optional
    }

    createInfo.preTransform = details.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = (m_initialized ? m_swapchain : VK_NULL_HANDLE);

    VULKAN_CHECK_CRITICAL(vkCreateSwapchainKHR(m_vkctx.device(), &createInfo, nullptr, &m_swapchain));

    /* Get images */
    VULKAN_CHECK_CRITICAL(vkGetSwapchainImagesKHR(m_vkctx.device(), m_swapchain, &imageCount, nullptr));
    m_swapchainImages.resize(imageCount);
    VULKAN_CHECK_CRITICAL(vkGetSwapchainImagesKHR(m_vkctx.device(), m_swapchain, &imageCount, m_swapchainImages.data()));

    m_format = surfaceFormat.format;
    m_extent = extent;

    VULKAN_CHECK_CRITICAL(createImageViews());
    VULKAN_CHECK_CRITICAL(createDepthBuffer());
    VULKAN_CHECK_CRITICAL(createFrameBufferAttachments());

    m_initialized = true;

    return VK_SUCCESS;
}

void VulkanSwapchain::releaseResources()
{
    for (auto imageView : m_depthImages) {
        imageView.destroy(m_vkctx.device());
    }

    for (auto imageView : m_swapchainImageViews) {
        vkDestroyImageView(m_vkctx.device(), imageView, nullptr);
    }

    m_framebufferAttachmentSwapchain.destroy(m_vkctx.device());
    m_framebufferAttachmentDepth.destroy(m_vkctx.device());

    vkDestroySwapchainKHR(m_vkctx.device(), m_swapchain, nullptr);

    m_initialized = false;
}

VkResult VulkanSwapchain::createImageViews()
{
    /* Create image views for the swapchain images */
    m_swapchainImageViews.resize(m_swapchainImages.size());
    for (uint32_t i = 0; i < m_swapchainImageViews.size(); i++) {
        VkImageViewCreateInfo imageViewInfo =
            vkinit::imageViewCreateInfo(m_swapchainImages[i], m_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        VULKAN_CHECK_CRITICAL(vkCreateImageView(m_vkctx.device(), &imageViewInfo, nullptr, &m_swapchainImageViews[i]));
    }

    return VK_SUCCESS;
}

VkResult VulkanSwapchain::createDepthBuffer()
{
    m_depthFormat = findDepthFormat();

    m_depthImages.resize(m_swapchainImageViews.size());
    for (uint32_t i = 0; i < m_swapchainImageViews.size(); i++) {
        VkImageCreateInfo imageInfo = vkinit::imageCreateInfo(
            {m_extent.width, m_extent.height, 1},
            m_depthFormat,
            1,
            VK_SAMPLE_COUNT_1_BIT,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        VULKAN_CHECK_CRITICAL(createImage(m_vkctx.physicalDevice(),
                                          m_vkctx.device(),
                                          imageInfo,
                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                          m_depthImages[i].image(),
                                          m_depthImages[i].memory()));

        VkImageViewCreateInfo imageViewInfo =
            vkinit::imageViewCreateInfo(m_depthImages[i].image(), m_depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
        VULKAN_CHECK_CRITICAL(vkCreateImageView(m_vkctx.device(), &imageViewInfo, nullptr, &m_depthImages[i].view()));
    }

    return VK_SUCCESS;
}

VkResult VulkanSwapchain::createFrameBufferAttachments()
{
    {
        std::vector<VkImageView> swapchainViews;
        for (size_t i = 0; i < imageCount(); i++) {
            swapchainViews.push_back(swapchainImageView(i));
        }

        VulkanFrameBufferAttachmentInfo swapchainInfo("swapchain", extent().width, extent().height, format(), VK_SAMPLE_COUNT_1_BIT);
        VULKAN_CHECK_CRITICAL(m_framebufferAttachmentSwapchain.init(swapchainInfo, swapchainViews));
    }

    {
        std::vector<VkImageView> depthViews;
        for (size_t i = 0; i < imageCount(); i++) {
            depthViews.push_back(depthStencilImageView(i));
        }
        VulkanFrameBufferAttachmentInfo depthInfo("depth", extent().width, extent().height, depthFormat(), VK_SAMPLE_COUNT_1_BIT);
        VULKAN_CHECK_CRITICAL(m_framebufferAttachmentDepth.init(depthInfo, depthViews));
    }

    return VK_SUCCESS;
}

VkSurfaceFormatKHR VulkanSwapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
{
    for (const auto &availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR VulkanSwapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
{
    for (const auto &availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSwapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, uint32_t width, uint32_t height)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = {width, height};

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

VkFormat VulkanSwapchain::findDepthFormat()
{
    return findSupportedFormat(m_vkctx.physicalDevice(),
                               {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                               VK_IMAGE_TILING_OPTIMAL,
                               VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

}  // namespace vengine
