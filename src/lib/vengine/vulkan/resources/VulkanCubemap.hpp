#ifndef __VulkanCubemap_hpp__
#define __VulkanCubemap_hpp__

#include <core/Cubemap.hpp>

#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/common/VulkanStructs.hpp"

namespace vengine
{

class VulkanCubemap : public Cubemap
{
public:
    VulkanCubemap(const AssetInfo &info, std::string directory, VulkanCommandInfo vci);
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

    VkResult createCubemap(VulkanCommandInfo vci);
};

}  // namespace vengine

#endif