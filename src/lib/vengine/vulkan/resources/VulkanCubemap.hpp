#ifndef __VulkanCubemap_hpp__
#define __VulkanCubemap_hpp__

#include <core/Cubemap.hpp>

#include "vulkan/common/IncludeVulkan.hpp"

namespace vengine
{

class VulkanCubemap : public Cubemap
{
public:
    VulkanCubemap(const AssetInfo &info,
                  std::string directory,
                  VkPhysicalDevice physicalDevice,
                  VkDevice device,
                  VkQueue queue,
                  VkCommandPool commandPool);
    VulkanCubemap(const AssetInfo &info,
                  VkImage cubemapImage,
                  VkDeviceMemory m_cubemapMemory,
                  VkImageView m_cubemapImageView,
                  VkSampler m_cubemapSampler);

    VkImage image() const;
    VkDeviceMemory deviceMemory() const;
    VkImageView imageView() const;
    VkSampler sampler() const;

    void destroy(VkDevice device);

private:
    VkImage m_cubemapImage;
    VkDeviceMemory m_cubemapMemory;
    VkImageView m_cubemapImageView;
    VkSampler m_cubemapSampler;

    VkResult createCubemap(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkCommandPool commandPool);
};

}  // namespace vengine

#endif