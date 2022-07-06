#ifndef __VulkanFramebuffer_hpp__
#define __VulkanFramebuffer_hpp__

#include "IncludeVulkan.hpp"

class VulkanFrameBufferAttachment {
public:

	bool init(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t width, uint32_t height, VkFormat format);

	void destroy(VkDevice device);

	VkImageView getView() const;

private:
	VkImage m_image;
	VkDeviceMemory m_memory;
	VkImageView m_view;
	VkFormat m_format;
};

#endif