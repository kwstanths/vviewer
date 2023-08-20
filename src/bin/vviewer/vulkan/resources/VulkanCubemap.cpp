#include "VulkanCubemap.hpp"

#include <debug_tools/Console.hpp>

#include "vulkan/common/VulkanInitializers.hpp"
#include "vulkan/common/VulkanUtils.hpp"

namespace vengine
{

VulkanCubemap::VulkanCubemap(std::string directory,
                             VkPhysicalDevice physicalDevice,
                             VkDevice device,
                             VkQueue queue,
                             VkCommandPool commandPool)
    : Cubemap(directory)
{
    bool ret = createCubemap(physicalDevice, device, queue, commandPool);
    if (!ret)
        throw std::runtime_error("Failed to create a vulkan cubemap");
}

VulkanCubemap::VulkanCubemap(std::string name,
                             VkImage cubemapImage,
                             VkDeviceMemory cubemapMemory,
                             VkImageView cubemapImageView,
                             VkSampler cubemapSampler)
    : m_cubemapImage(cubemapImage)
    , m_cubemapMemory(cubemapMemory)
    , m_cubemapImageView(cubemapImageView)
    , m_cubemapSampler(cubemapSampler)
{
    m_name = name;
}

VkImage VulkanCubemap::getImage() const
{
    return m_cubemapImage;
}

VkDeviceMemory VulkanCubemap::getDeviceMemory() const
{
    return m_cubemapMemory;
}

VkImageView VulkanCubemap::getImageView() const
{
    return m_cubemapImageView;
}

VkSampler VulkanCubemap::getSampler() const
{
    return m_cubemapSampler;
}

void VulkanCubemap::destroy(VkDevice device)
{
    vkDestroyImageView(device, m_cubemapImageView, nullptr);
    vkDestroyImage(device, m_cubemapImage, nullptr);
    vkFreeMemory(device, m_cubemapMemory, nullptr);
    vkDestroySampler(device, m_cubemapSampler, nullptr);
}

VkResult VulkanCubemap::createCubemap(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkCommandPool commandPool)
{
    int width = m_image_top->getWidth();
    int height = m_image_top->getHeight();
    int channels = m_image_top->getChannels();

    const VkDeviceSize imageSize = width * height * channels * 6;
    const VkDeviceSize layerSize = imageSize / 6;

    /* Create staging buffer */
    VulkanBuffer stagingBuffer;
    VULKAN_CHECK_CRITICAL(createBuffer(physicalDevice,
                                       device,
                                       imageSize,
                                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       stagingBuffer));

    /* Copy data to staging buffer */
    void *data;
    VULKAN_CHECK_CRITICAL(vkMapMemory(device, stagingBuffer.memory(), 0, imageSize, 0, &data));
    memcpy(static_cast<stbi_uc *>(data) + (layerSize * 0), m_image_front->getData(), static_cast<size_t>(layerSize));
    memcpy(static_cast<stbi_uc *>(data) + (layerSize * 1), m_image_back->getData(), static_cast<size_t>(layerSize));
    memcpy(static_cast<stbi_uc *>(data) + (layerSize * 2), m_image_top->getData(), static_cast<size_t>(layerSize));
    memcpy(static_cast<stbi_uc *>(data) + (layerSize * 3), m_image_bottom->getData(), static_cast<size_t>(layerSize));
    memcpy(static_cast<stbi_uc *>(data) + (layerSize * 4), m_image_right->getData(), static_cast<size_t>(layerSize));
    memcpy(static_cast<stbi_uc *>(data) + (layerSize * 5), m_image_left->getData(), static_cast<size_t>(layerSize));
    vkUnmapMemory(device, stagingBuffer.memory());

    // Create optimal tiled target image
    VkImageCreateInfo imageCreateInfo = vkinit::imageCreateInfo({static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1},
                                                                VK_FORMAT_R8G8B8A8_SRGB,
                                                                1,
                                                                VK_SAMPLE_COUNT_1_BIT,
                                                                VK_IMAGE_TILING_OPTIMAL,
                                                                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    imageCreateInfo.arrayLayers = 6;                             /* // Cube faces count as array layers */
    imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT; /* This flag is required for cube map images */

    VULKAN_CHECK_CRITICAL(
        createImage(physicalDevice, device, imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_cubemapImage, m_cubemapMemory));

    /* Transition image to DST_OPTIMAL in order to transfer the data from the staging buffer */
    VULKAN_CHECK_CRITICAL(transitionImageLayout(
        device, queue, commandPool, m_cubemapImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6));

    /* copy data fronm staging buffer to image */
    std::vector<VkBufferImageCopy> copyRegions(6);
    for (int i = 0; i < 6; i++) {
        copyRegions[i].bufferOffset = layerSize * i;
        copyRegions[i].bufferRowLength = 0;
        copyRegions[i].bufferImageHeight = 0;
        copyRegions[i].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegions[i].imageSubresource.mipLevel = 0;
        copyRegions[i].imageSubresource.baseArrayLayer = i;
        copyRegions[i].imageSubresource.layerCount = 1;
        copyRegions[i].imageOffset = {0, 0, 0};
        copyRegions[i].imageExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
    }
    VULKAN_CHECK_CRITICAL(copyBufferToImage(device, queue, commandPool, stagingBuffer.buffer(), m_cubemapImage, copyRegions));

    /* Transition to SHADER_READ_ONLY layout */
    VULKAN_CHECK_CRITICAL(transitionImageLayout(device,
                                                queue,
                                                commandPool,
                                                m_cubemapImage,
                                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                6));

    /* Cleanup staging buffer */
    stagingBuffer.destroy(device);

    /* Create image view */
    VkImageViewCreateInfo viewInfo =
        vkinit::imageViewCreateInfo(m_cubemapImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    viewInfo.subresourceRange.layerCount = 6;
    VULKAN_CHECK_CRITICAL(vkCreateImageView(device, &viewInfo, nullptr, &m_cubemapImageView));

    VkSamplerCreateInfo samplerInfo = vkinit::samplerCreateInfo(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR);
    VULKAN_CHECK_CRITICAL(vkCreateSampler(device, &samplerInfo, nullptr, &m_cubemapSampler));

    return VK_SUCCESS;
}

}  // namespace vengine