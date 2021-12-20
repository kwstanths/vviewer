#include "VulkanCubemap.hpp"

#include <utils/Console.hpp>

#include "VulkanUtils.hpp"

VulkanCubemap::VulkanCubemap(std::string directory, VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkCommandPool commandPool) : Cubemap(directory)
{
    bool ret = createCubemap(physicalDevice, device, queue, commandPool);
    if (!ret) throw std::runtime_error("Failed to create a vulkan cubemap");
}

VulkanCubemap::VulkanCubemap(std::string name, VkImage cubemapImage, VkDeviceMemory cubemapMemory, VkImageView cubemapImageView, VkSampler cubemapSampler):
    m_cubemapImage(cubemapImage), m_cubemapMemory(cubemapMemory), m_cubemapImageView(cubemapImageView), m_cubemapSampler(cubemapSampler)
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

bool VulkanCubemap::createCubemap(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkCommandPool commandPool)
{
    int width = m_image_top->getWidth();
    int height = m_image_top->getHeight();
    int channels = m_image_top->getChannels();

    const VkDeviceSize imageSize = width * height * channels * 6;
    const VkDeviceSize layerSize = imageSize / 6;

    /* Create staging buffer */
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(physicalDevice, device, imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory);

    /* Copy data to staging buffer */
    void *data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(static_cast<stbi_uc *>(data) + (layerSize * 0), m_image_front->getData(), static_cast<size_t>(layerSize));
    memcpy(static_cast<stbi_uc *>(data) + (layerSize * 1), m_image_back->getData(), static_cast<size_t>(layerSize));
    memcpy(static_cast<stbi_uc *>(data) + (layerSize * 2), m_image_top->getData(), static_cast<size_t>(layerSize));
    memcpy(static_cast<stbi_uc *>(data) + (layerSize * 3), m_image_bottom->getData(), static_cast<size_t>(layerSize));
    memcpy(static_cast<stbi_uc *>(data) + (layerSize * 4), m_image_right->getData(), static_cast<size_t>(layerSize));
    memcpy(static_cast<stbi_uc *>(data) + (layerSize * 5), m_image_left->getData(), static_cast<size_t>(layerSize));
    vkUnmapMemory(device, stagingBufferMemory);

    // Create optimal tiled target image
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    // Cube faces count as array layers in Vulkan
    imageCreateInfo.arrayLayers = 6;
    // This flag is required for cube map images
    imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    VkResult ret = vkCreateImage(device, &imageCreateInfo, nullptr, &m_cubemapImage);
    if (ret != VK_SUCCESS) {
        utils::ConsoleCritical("Failed to create a vulkan cubemap image");
        return false;
    }

    /* Allocate required memory for cubemap */
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, m_cubemapImage, &memRequirements);
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(device, &allocInfo, nullptr, &m_cubemapMemory) != VK_SUCCESS) {
        utils::ConsoleCritical("Failed to allocate memory for a vulkan cubemap");
        return false;
    }
    vkBindImageMemory(device, m_cubemapImage, m_cubemapMemory, 0);

    /* Transition image to DST_OPTIMAL in order to transfer the data from the staging buffer */
    transitionImageLayout(device, queue, commandPool,
        m_cubemapImage,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        6);

    /* copy data fronm staging buffer to image */
    std::vector< VkBufferImageCopy> copyRegions(6);
    for (int i = 0; i < 6; i++) {
        copyRegions[i].bufferOffset = layerSize * i;
        copyRegions[i].bufferRowLength = 0;
        copyRegions[i].bufferImageHeight = 0;
        copyRegions[i].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegions[i].imageSubresource.mipLevel = 0;
        copyRegions[i].imageSubresource.baseArrayLayer = i;
        copyRegions[i].imageSubresource.layerCount = 1;
        copyRegions[i].imageOffset = { 0, 0, 0 };
        copyRegions[i].imageExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height),
            1
        };
    }
    copyBufferToImage(device, queue, commandPool, stagingBuffer, m_cubemapImage, copyRegions);

    /* Transition to SHADER_READ_ONLY layout */
    transitionImageLayout(device, queue, commandPool,
        m_cubemapImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        6);

    /* Cleanup staging buffer */
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);

    /* Create image view */
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_cubemapImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 6;
    if (vkCreateImageView(device, &viewInfo, nullptr, &m_cubemapImageView) != VK_SUCCESS) {
        utils::ConsoleCritical("Failed to create an image view for a cubemap");
        return false;
    }

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 0.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &m_cubemapSampler) != VK_SUCCESS) {
        utils::ConsoleCritical("Failed to create a texture sampler for a cubemap");
        return false;
    }

    return true;
}
