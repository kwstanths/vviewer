#ifndef __VulkanFramebuffer_hpp__
#define __VulkanFramebuffer_hpp__

#include "IncludeVulkan.hpp"

class VulkanFrameBufferAttachment {
public:

	bool init(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags extraUsageFlags = 0);

	void destroy(VkDevice device);

	VkImageView getView() const;
	VkImage getImage() const;

private:
	VkImage m_image;
	VkDeviceMemory m_memory;
	VkImageView m_view;
	VkFormat m_format;
};

#endif