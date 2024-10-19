#include "VulkanStructs.hpp"

#include "IncludeVulkan.hpp"

namespace vengine
{

bool VulkanQueueFamilyInfo::hasPresent() const
{
    return present;
}

bool VulkanQueueFamilyInfo::hasGraphics() const
{
    return properties.queueFlags & VK_QUEUE_GRAPHICS_BIT;
}

bool VulkanQueueFamilyInfo::hasCompute() const
{
    return properties.queueFlags & VK_QUEUE_COMPUTE_BIT;
}

bool VulkanQueueFamilyInfo::hasTransfer() const
{
    return properties.queueFlags & VK_QUEUE_TRANSFER_BIT;
}

}  // namespace vengine
