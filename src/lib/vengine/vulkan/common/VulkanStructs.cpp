#include "VulkanStructs.hpp"

#include "IncludeVulkan.hpp"

namespace vengine
{

bool VulkanQueueFamilyIndices::hasGraphics() const
{
    return graphicsFamily.has_value();
}

bool VulkanQueueFamilyIndices::hasPresent() const
{
    return presentFamily.has_value();
}

bool VulkanQueueFamilyIndices::isComplete() const
{
    return hasGraphics() && hasPresent();
}

}  // namespace vengine
