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
    VulkanTexture(const Image<stbi_uc> &image, VulkanCommandInfo vci, bool genMipMaps);

    VulkanTexture(const Image<float> &image, VulkanCommandInfo vci, bool genMipMaps);

    VulkanTexture(const AssetInfo &info,
                  ColorSpace colorSpace,
                  ColorDepth colorDepth,
                  size_t width,
                  size_t height,
                  size_t channels,
                  VkImage &image,
                  VkDeviceMemory &imageMemory,
                  VkImageView &imageView,
                  VkSampler &sampler,
                  VkFormat &format,
                  uint32_t numMips);

    void destroy(VkDevice device);

    const VkImage &image() const;
    const VkImageView &imageView() const;
    const VkDeviceMemory &imageMemory() const;
    const VkFormat &format() const;
    const VkSampler &sampler() const;

    void setBindlessResourceIndex(int32_t index);
    int32_t getBindlessResourceIndex() const;

private:
    VkImage m_image;
    VkDeviceMemory m_imageMemory;
    VkImageView m_imageView;
    VkSampler m_sampler;

    VkFormat m_format;
    uint32_t m_numMips = 1;

    int32_t m_bindlessResourceIndex = -1;

    VkResult createSampler(VkDevice device, VkSampler &sampler) const;
    void generateMipMaps(VulkanCommandInfo vci);
};

}  // namespace vengine

#endif
