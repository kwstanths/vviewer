#ifndef __VulkanTexture_hpp__
#define __VulkanTexture_hpp__

#include <memory>

#include <core/Image.hpp>
#include <core/Texture.hpp>

#include "IncludeVulkan.hpp"

class VulkanTexture : public Texture {
public:
    VulkanTexture(std::string name, Image<stbi_uc> * image, TextureType type, VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkCommandPool commandPool, VkFormat format);
    VulkanTexture(std::string name, Image<float> * image, TextureType type, VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkCommandPool commandPool);
    /* unused */
    VulkanTexture(std::string name, TextureType type, VkFormat format, size_t width, size_t height, VkImage& image, VkDeviceMemory& imageMemory, VkImageView& imageView);

    VkImage getImage() const;
    VkImageView getImageView() const;
    VkDeviceMemory getImageMemory() const;
    VkFormat getFormat() const;

private:
    VkImage m_image;
    VkDeviceMemory m_imageMemory;
    VkImageView m_imageView;
    VkFormat m_format;
    uint32_t m_numMips;
};

#endif
