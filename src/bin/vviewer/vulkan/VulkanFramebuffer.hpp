#ifndef __VulkanFramebuffer_hpp__
#define __VulkanFramebuffer_hpp__

#include <vector>

#include "IncludeVulkan.hpp"

class VulkanFramebuffer {
public:

	void initSwapchainResources(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t width, uint32_t height, VkFormat format, uint32_t count);
	void releaseSwapchainResources();

private:
	VkPhysicalDevice m_physicalDevice;
	VkDevice m_device;

	std::vector<VkImage> m_image;
	std::vector<VkImageView> m_imageView;
	std::vector<VkDeviceMemory> m_imageMemory;
};

#endif