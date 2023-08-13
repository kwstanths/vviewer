#include "VulkanSwapchain.hpp"

#include "VulkanUtils.hpp"

VulkanSwapchain::VulkanSwapchain(VulkanContext& vkctx) : m_vkctx(vkctx)
{

}

VulkanSwapchain::~VulkanSwapchain()
{
}

bool VulkanSwapchain::initResources(uint32_t width, uint32_t height)
{
    SwapChainDetails details = querySwapChainSupport(m_vkctx.physicalDevice(), m_vkctx.surface());

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

    QueueFamilyIndices& indices = m_vkctx.queueFamilyIndices();
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = details.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = (m_initialized? m_swapchain : VK_NULL_HANDLE);

    VkResult ret = vkCreateSwapchainKHR(m_vkctx.device(), &createInfo, nullptr, &m_swapchain);
    if (ret != VK_SUCCESS) {
       throw std::runtime_error("VulkanSwapchain::init(): Failed to create a swap chain: " + std::to_string(ret));
    }

    /* Get images */
    vkGetSwapchainImagesKHR(m_vkctx.device(), m_swapchain, &imageCount, nullptr);
    m_swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_vkctx.device(), m_swapchain, &imageCount, m_swapchainImages.data());

    m_format = surfaceFormat.format;
    m_extent = extent;

    createImageViews();
    createDepthBuffer();

    m_initialized = true;
    return true;
}

void VulkanSwapchain::releaseResources()
{
    vkDestroyImageView(m_vkctx.device(), m_msaaImageView, nullptr);
    vkDestroyImage(m_vkctx.device(), m_msaaImage, nullptr);
    vkFreeMemory(m_vkctx.device(), m_msaaImageMemory, nullptr);

    vkDestroyImageView(m_vkctx.device(), m_depthImageView, nullptr);
    vkDestroyImage(m_vkctx.device(), m_depthImage, nullptr);
    vkFreeMemory(m_vkctx.device(), m_depthImageMemory, nullptr);

    for (auto imageView : m_swapchainImageViews) {
        vkDestroyImageView(m_vkctx.device(), imageView, nullptr);
    }

    vkDestroySwapchainKHR(m_vkctx.device(), m_swapchain, nullptr);

    m_initialized = false;
}

void VulkanSwapchain::createImageViews()
{
    /* Create image views for the swapchain images */
    m_swapchainImageViews.resize(m_swapchainImages.size());
    for(uint32_t i=0; i < m_swapchainImageViews.size(); i++)
    {
        m_swapchainImageViews[i] = createImageView(m_vkctx.device(), m_swapchainImages[i], m_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }

    /* Create images and image views for the MSAA targets */
    createImage(m_vkctx.physicalDevice(), 
        m_vkctx.device(), 
        m_extent.width, 
        m_extent.height, 
        1, 
        m_vkctx.msaaSamples(), 
        m_format, 
        VK_IMAGE_TILING_OPTIMAL, 
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_msaaImage, m_msaaImageMemory);

    m_msaaImageView = createImageView(m_vkctx.device(), m_msaaImage, m_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

void VulkanSwapchain::createDepthBuffer()
{
    m_depthFormat = findDepthFormat();

    createImage(m_vkctx.physicalDevice(), 
        m_vkctx.device(), 
        m_extent.width, 
        m_extent.height, 
        1, 
        m_vkctx.msaaSamples(), 
        m_depthFormat, 
        VK_IMAGE_TILING_OPTIMAL, 
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
        m_depthImage, 
        m_depthImageMemory
    );
        
    m_depthImageView = createImageView(m_vkctx.device(), m_depthImage, m_depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}

VkSurfaceFormatKHR VulkanSwapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR VulkanSwapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSwapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = { width, height };

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
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

