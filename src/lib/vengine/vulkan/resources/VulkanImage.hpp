#ifndef __VulkanImage_hpp__
#define __VulkanImage_hpp__

#include "vulkan/common/IncludeVulkan.hpp"

namespace vengine
{

/**
 * @brief Represents a VkImage, its view and its memory
 *
 */
class VulkanImage
{
public:
    VulkanImage();
    VulkanImage(VkImageView view, VkFormat format);
    VulkanImage(VkImage image, VkImageView view, VkDeviceMemory memory, VkFormat format);

    void destroy(VkDevice device);

    inline VkImage &image() { return m_image; }
    inline const VkImage &image() const { return m_image; }

    inline VkImageView &view() { return m_view; }
    inline const VkImageView &view() const { return m_view; }

    inline VkFormat &format() { return m_format; }
    inline const VkFormat &format() const { return m_format; }

    inline VkDeviceMemory &memory() { return m_memory; }
    inline const VkDeviceMemory &memory() const { return m_memory; }

private:
    VkImage m_image = VK_NULL_HANDLE;
    VkImageView m_view = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkFormat m_format = {};
};

}  // namespace vengine

#endif
