#include "VulkanDataStructs.hpp"

#include "IncludeVulkan.hpp"

void StorageImage::destroy(VkDevice device)
{
	vkDestroyImage(device, image, nullptr);
	vkDestroyImageView(device, view, nullptr);
	vkFreeMemory(device, memory, nullptr);
}
