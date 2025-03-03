#ifndef __VulkanTexture_hpp__
#define __VulkanTexture_hpp__

#include <memory>

#include <core/Image.hpp>
#include <core/Texture.hpp>

#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/common/VulkanStructs.hpp"

namespace vengine
{

class VulkanTexture : public Texture
{
public:
    VulkanTexture(const Image &image, VulkanCommandInfo vci);

    VulkanTexture(const AssetInfo &info,
                  size_t width,
                  size_t height,
                  Format format,
                  ColorSpace colorSpace,
                  VkImage &image,
                  VkDeviceMemory &imageMemory,
                  VkImageView &imageView,
                  VkSampler &sampler,
                  VkFormat &vkformat,
                  uint32_t numMips);

    void destroy(VkDevice device);

    const VkImage &image() const;
    const VkImageView &imageView() const;
    const VkDeviceMemory &imageMemory() const;
    const VkFormat &format() const;
    const VkSampler &sampler() const;

    const uint32_t &bindlessResourceIndex() const { return m_bindlessResourceIndex; }
    uint32_t &bindlessResourceIndex() { return m_bindlessResourceIndex; }

private:
    VkImage m_image;
    VkDeviceMemory m_imageMemory;
    VkImageView m_imageView;
    VkSampler m_sampler;

    VkFormat m_vkformat;
    uint32_t m_numMips = 1;

    uint32_t m_bindlessResourceIndex = 0;

    VkResult createSampler(VkDevice device, VkSampler &sampler) const;
    void generateMipMaps(VulkanCommandInfo vci);
};

}  // namespace vengine

#endif
