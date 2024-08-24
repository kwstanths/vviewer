#ifndef __VulkanSSBO_hpp__
#define __VulkanSSBO_hpp__

#include "vulkan/resources/VulkanBufferObject.hpp"

namespace vengine
{

/**
    A class to manage a GPU storage buffer and its CPU space transfer memory that stores data of type Block
*/
template <typename Block>
class VulkanSSBO : public VulkanBufferObject<Block>
{
public:
    VulkanSSBO(uint32_t nBlocks)
        : VulkanBufferObject<Block>(nBlocks)
    {
    }

    VkResult init(const VkPhysicalDeviceProperties &physicalDeviceProperties)
    {
        return VulkanBufferObject<Block>::init(static_cast<uint32_t>(physicalDeviceProperties.limits.minStorageBufferOffsetAlignment));
    }

    VkResult createBuffers(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t nBuffers)
    {
        return VulkanBufferObject<Block>::createBuffers(physicalDevice, device, nBuffers, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    }
};

}  // namespace vengine

#endif
