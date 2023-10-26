#include "VulkanStructs.hpp"

#include "IncludeVulkan.hpp"

namespace vengine
{

bool QueueFamilyIndices::hasGraphics() const
{
    return graphicsFamily.has_value();
}

bool QueueFamilyIndices::hasPresent() const
{
    return presentFamily.has_value();
}

bool QueueFamilyIndices::isComplete() const
{
    return hasGraphics() && hasPresent();
}

void StorageImage::destroy(VkDevice device)
{
    if (image != VK_NULL_HANDLE)
        vkDestroyImage(device, image, nullptr);
    if (view != VK_NULL_HANDLE)
        vkDestroyImageView(device, view, nullptr);
    if (memory != VK_NULL_HANDLE)
        vkFreeMemory(device, memory, nullptr);
}

}  // namespace vengine