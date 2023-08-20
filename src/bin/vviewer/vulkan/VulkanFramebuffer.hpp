#ifndef __VulkanFramebuffer_hpp__
#define __VulkanFramebuffer_hpp__

#include "vulkan/common/IncludeVulkan.hpp"

namespace vengine {

class VulkanFrameBufferAttachment {
public:

	bool init(VkPhysicalDevice physicalDevice, 
		VkDevice device, 
		uint32_t width, 
		uint32_t height, 
		VkFormat format, 
		VkSampleCountFlagBits numSamples,
		bool createResolveAttachment = false,
		VkImageUsageFlags extraUsageFlags = 0
	);
	
	void destroy(VkDevice device);

	VkImageView getView() const;
	VkImage getImage() const;

	bool hasResolveTargets() const;
	
	VkImageView getViewResolve() const;
	VkImage getImageResolve() const;

private:
	VkImage m_image;
	VkDeviceMemory m_memory;
	VkImageView m_view;

	bool m_hasResolveAttachment;
	VkImage m_imageResolve;
	VkDeviceMemory m_memoryResolve;
	VkImageView m_viewResolve;

	VkFormat m_format;
};

}

#endif