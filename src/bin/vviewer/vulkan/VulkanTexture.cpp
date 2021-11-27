#include "VulkanTexture.hpp"

#include "VulkanUtils.hpp"
#include "utils/Console.hpp"

VulkanTexture::VulkanTexture(std::string name, Image<stbi_uc> * image, TextureType type, VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkCommandPool commandPool, VkFormat format)
    :Texture(name, type)
{
    int imageWidth = image->getWidth();
    int imageHeight = image->getHeight();

    /* Size of image in bytes  */
    VkDeviceSize imageSize = imageWidth * imageHeight * image->getChannels();

    /* Create staging buffer */
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(physicalDevice, device, imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory);

    /* Copy image data to staging buffer */
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, image->getData(), static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    /* Create vulkan image and memory */
    createImage(physicalDevice, device, imageWidth,
        imageHeight,
        format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_image,
        m_imageMemory);

    /* Transition image to DST_OPTIMAL in order to transfer the data */
    transitionImageLayout(device, queue, commandPool,
        m_image,
        format,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    /* copy data fronm staging buffer to image */
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = {
        static_cast<uint32_t>(imageWidth),
        static_cast<uint32_t>(imageHeight),
        1
    };
    copyBufferToImage(device, queue, commandPool, stagingBuffer, m_image, { region });

    /* Transition to SHADER_READ_ONLY layout */
    transitionImageLayout(device, queue, commandPool,
        m_image,
        format,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    /* Cleanup staging buffer */
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);

    /* Create view */
    m_imageView = createImageView(device, m_image, format, VK_IMAGE_ASPECT_COLOR_BIT);
}


VulkanTexture::VulkanTexture(std::string name, Image<float> * image, TextureType type, VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkCommandPool commandPool)
    :Texture(name, type)
{
    int imageWidth = image->getWidth();
    int imageHeight = image->getHeight();

    VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
    /* Size of image in bytes  */
    VkDeviceSize imageSize = imageWidth * imageHeight * image->getChannels() * 4;

    /* Create staging buffer */
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(physicalDevice, device, imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory);

    /* Copy image data to staging buffer */
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, image->getData(), static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    /* Create vulkan image and memory */
    createImage(physicalDevice, device, imageWidth,
        imageHeight,
        format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_image,
        m_imageMemory);

    /* Transition image to DST_OPTIMAL in order to transfer the data */
    transitionImageLayout(device, queue, commandPool,
        m_image,
        format,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    /* copy data fronm staging buffer to image */
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = {
        static_cast<uint32_t>(imageWidth),
        static_cast<uint32_t>(imageHeight),
        1
    };
    copyBufferToImage(device, queue, commandPool, stagingBuffer, m_image, { region });

    /* Transition to SHADER_READ_ONLY layout */
    transitionImageLayout(device, queue, commandPool,
        m_image,
        format,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    /* Cleanup staging buffer */
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);

    /* Create view */
    m_imageView = createImageView(device, m_image, format, VK_IMAGE_ASPECT_COLOR_BIT);
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
