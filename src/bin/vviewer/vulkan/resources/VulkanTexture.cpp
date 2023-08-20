#include "VulkanTexture.hpp"

#include "vulkan/common/VulkanInitializers.hpp"
#include "vulkan/common/VulkanUtils.hpp"
#include "debug_tools/Console.hpp"

namespace vengine
{

VulkanTexture::VulkanTexture(std::string name,
                             std::shared_ptr<Image<stbi_uc>> image,
                             TextureType type,
                             VkPhysicalDevice physicalDevice,
                             VkDevice device,
                             VkQueue queue,
                             VkCommandPool commandPool,
                             VkFormat format,
                             bool genMipMaps)
    : Texture(name, type, image->getWidth(), image->getHeight())
{
    uint32_t imageWidth = image->getWidth();
    uint32_t imageHeight = image->getHeight();

    if (genMipMaps) {
        m_numMips = static_cast<uint32_t>(std::floor(std::log2(std::max(m_width, m_height)))) + 1;
    }

    m_format = format;
    /* Size of image in bytes  */
    VkDeviceSize imageSize = imageWidth * imageHeight * image->getChannels();

    /* Create staging buffer */
    VulkanBuffer stagingBuffer;
    createBuffer(physicalDevice,
                 device,
                 imageSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer);

    /* Copy image data to staging buffer */
    void *data;
    vkMapMemory(device, stagingBuffer.memory(), 0, imageSize, 0, &data);
    memcpy(data, image->getData(), static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBuffer.memory());

    /* Create vulkan image and memory */
    VkImageUsageFlags imageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    if (genMipMaps) {
        /* If mip maps are to be generated, it will be used a transfer src as well */
        imageFlags = imageFlags | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    VkImageCreateInfo imageInfo = vkinit::imageCreateInfo(
        {imageWidth, imageHeight, 1}, m_format, m_numMips, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, imageFlags);
    createImage(physicalDevice, device, imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_image, m_imageMemory);

    /* Transition image to DST_OPTIMAL in order to transfer the data */
    transitionImageLayout(
        device, queue, commandPool, m_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_numMips);

    /* copy data fronm staging buffer to image */
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {static_cast<uint32_t>(imageWidth), static_cast<uint32_t>(imageHeight), 1};
    copyBufferToImage(device, queue, commandPool, stagingBuffer.buffer(), m_image, {region});

    if (genMipMaps) {
        /* If mip maps are to be generated, transition to shader read only while creating the mip maps */
        generateMipMaps(device, commandPool, queue);
    } else {
        /* Transition to SHADER_READ_ONLY layout */
        transitionImageLayout(device,
                              queue,
                              commandPool,
                              m_image,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              m_numMips);
    }

    /* Cleanup staging buffer */
    stagingBuffer.destroy(device);

    /* Create view */
    VkImageViewCreateInfo imageViewInfo = vkinit::imageViewCreateInfo(m_image, m_format, VK_IMAGE_ASPECT_COLOR_BIT, m_numMips);
    vkCreateImageView(device, &imageViewInfo, nullptr, &m_imageView);

    /* Create a sampler */
    createSampler(device, m_sampler);
}

VulkanTexture::VulkanTexture(std::string name,
                             std::shared_ptr<Image<float>> image,
                             TextureType type,
                             VkPhysicalDevice physicalDevice,
                             VkDevice device,
                             VkQueue queue,
                             VkCommandPool commandPool,
                             bool genMipMaps)
    : Texture(name, type, image->getWidth(), image->getHeight())
{
    uint32_t imageWidth = image->getWidth();
    uint32_t imageHeight = image->getHeight();

    if (genMipMaps) {
        m_numMips = static_cast<uint32_t>(std::floor(std::log2(std::max(m_width, m_height)))) + 1;
    }

    m_format = VK_FORMAT_R32G32B32A32_SFLOAT;
    /* Size of image in bytes  */
    VkDeviceSize imageSize = imageWidth * imageHeight * image->getChannels() * 4;

    /* Create staging buffer */
    VulkanBuffer stagingBuffer;
    createBuffer(physicalDevice,
                 device,
                 imageSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer);

    /* Copy image data to staging buffer */
    void *data;
    vkMapMemory(device, stagingBuffer.memory(), 0, imageSize, 0, &data);
    memcpy(data, image->getData(), static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBuffer.memory());

    /* Create vulkan image and memory */
    VkImageUsageFlags imageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    if (genMipMaps) {
        /* If mip maps are to be generated, it will be used a transfer src as well */
        imageFlags = imageFlags | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    VkImageCreateInfo imageInfo = vkinit::imageCreateInfo(
        {imageWidth, imageHeight, 1}, m_format, m_numMips, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, imageFlags);
    createImage(physicalDevice, device, imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_image, m_imageMemory);

    /* Transition image to DST_OPTIMAL in order to transfer the data */
    transitionImageLayout(
        device, queue, commandPool, m_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_numMips);

    /* copy data fronm staging buffer to image */
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {static_cast<uint32_t>(imageWidth), static_cast<uint32_t>(imageHeight), 1};
    copyBufferToImage(device, queue, commandPool, stagingBuffer.buffer(), m_image, {region});

    if (genMipMaps) {
        /* If mip maps are to be generated, transition to shader read only while creating the mip maps */
        generateMipMaps(device, commandPool, queue);
    } else {
        /* Transition to SHADER_READ_ONLY layout */
        transitionImageLayout(device,
                              queue,
                              commandPool,
                              m_image,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              m_numMips);
    }

    /* Cleanup staging buffer */
    stagingBuffer.destroy(device);

    /* Create view */
    VkImageViewCreateInfo imageViewInfo = vkinit::imageViewCreateInfo(m_image, m_format, VK_IMAGE_ASPECT_COLOR_BIT, m_numMips);
    vkCreateImageView(device, &imageViewInfo, nullptr, &m_imageView);

    /* Create a sampler */
    createSampler(device, m_sampler);
}

VulkanTexture::VulkanTexture(std::string name,
                             TextureType type,
                             VkFormat format,
                             size_t width,
                             size_t height,
                             VkImage &image,
                             VkDeviceMemory &imageMemory,
                             VkImageView &imageView,
                             VkSampler &sampler,
                             uint32_t numMips)
    : Texture(name, type, width, height)
    , m_image(image)
    , m_imageMemory(imageMemory)
    , m_imageView(imageView)
    , m_sampler(sampler)
    , m_format(format)
    , m_numMips(numMips)
{
}

void VulkanTexture::destroy(VkDevice device)
{
    vkDestroyImage(device, m_image, nullptr);
    vkDestroyImageView(device, m_imageView, nullptr);
    vkFreeMemory(device, m_imageMemory, nullptr);
    vkDestroySampler(device, m_sampler, nullptr);
}

VkImage VulkanTexture::getImage() const
{
    return m_image;
}

VkImageView VulkanTexture::getImageView() const
{
    return m_imageView;
}

VkDeviceMemory VulkanTexture::getImageMemory() const
{
    return m_imageMemory;
}

VkFormat VulkanTexture::getFormat() const
{
    return m_format;
}

VkSampler VulkanTexture::getSampler() const
{
    return m_sampler;
}

void VulkanTexture::setBindlessResourceIndex(int32_t index)
{
    m_bindlessResourceIndex = index;
}

int32_t VulkanTexture::getBindlessResourceIndex() const
{
    return m_bindlessResourceIndex;
}

VkResult VulkanTexture::createSampler(VkDevice device, VkSampler &sampler) const
{
    VkSamplerCreateInfo samplerInfo = vkinit::samplerCreateInfo(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR);
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.maxLod = static_cast<float>(m_numMips);

    VULKAN_CHECK_CRITICAL(vkCreateSampler(device, &samplerInfo, nullptr, &sampler));

    return VK_SUCCESS;
}

void VulkanTexture::generateMipMaps(VkDevice device, VkCommandPool commandPool, VkQueue queue)
{
    VkCommandBuffer cmdBuf;
    beginSingleTimeCommands(device, commandPool, cmdBuf);

    VkImageSubresourceRange resourceRange = {};
    resourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    resourceRange.baseArrayLayer = 0;
    resourceRange.layerCount = 1;
    resourceRange.levelCount = 1;

    for (uint32_t i = 1; i < m_numMips; i++) {
        /* Transition previous mip level to src layout */
        resourceRange.baseMipLevel = i - 1;
        transitionImageLayout(
            cmdBuf, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, resourceRange);

        /* Data for blit operation */
        VkImageBlit blit{};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {std::max(int32_t(m_width >> (i - 1)), 1), std::max(int32_t(m_height >> (i - 1)), 1), 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {std::max(int32_t(m_width >> i), 1), std::max(int32_t(m_height >> i), 1), 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(cmdBuf,
                       m_image,
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       m_image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1,
                       &blit,
                       VK_FILTER_LINEAR);

        /* Transition previous level to shader read only */
        transitionImageLayout(
            cmdBuf, m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, resourceRange);
    }

    /* Transition last mip level to shader read only layout */
    resourceRange.baseMipLevel = m_numMips - 1;
    transitionImageLayout(
        cmdBuf, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, resourceRange);

    endSingleTimeCommands(device, commandPool, queue, cmdBuf);
}

}  // namespace vengine