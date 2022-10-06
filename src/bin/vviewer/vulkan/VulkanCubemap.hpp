#ifndef __VulkanCubemap_hpp__
#define __VulkanCubemap_hpp__

#include <core/Cubemap.hpp>

#include "IncludeVulkan.hpp"

class VulkanCubemap : public Cubemap {
public:
    VulkanCubemap(std::string directory, VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkCommandPool commandPool);
    VulkanCubemap(std::string name, VkImage cubemapImage, VkDeviceMemory m_cubemapMemory, VkImageView m_cubemapImageView, VkSampler m_cubemapSampler);

    VkImage getImage() const;
    VkDeviceMemory getDeviceMemory() const;
    VkImageView getImageView() const;
    VkSampler getSampler() const;

    void destroy(VkDevice device);

private:
    VkImage m_cubemapImage;
    VkDeviceMemory m_cubemapMemory;
    VkImageView m_cubemapImageView;
    VkSampler m_cubemapSampler;

    bool createCubemap(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkCommandPool commandPool);
};

#endif