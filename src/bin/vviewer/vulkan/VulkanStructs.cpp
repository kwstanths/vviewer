#include "VulkanStructs.hpp"

#include "IncludeVulkan.hpp"

bool QueueFamilyIndices::isComplete() {
	return graphicsFamily.has_value() && presentFamily.has_value();
}

void StorageImage::destroy(VkDevice device)
{
	if (image != VK_NULL_HANDLE) vkDestroyImage(device, image, nullptr);
	if (view != VK_NULL_HANDLE) vkDestroyImageView(device, view, nullptr);
	if (memory != VK_NULL_HANDLE) vkFreeMemory(device, memory, nullptr);
}
