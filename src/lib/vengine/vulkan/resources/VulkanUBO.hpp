#ifndef __VulkanUBO_hpp__
#define __VulkanUBO_hpp__

#include "vulkan/resources/VulkanBufferObject.hpp"

namespace vengine
{

/**
    A class to manage a GPU uniform buffer and its CPU space transfer memory that stores data of type Block
*/
template <typename Block>
class VulkanUBO : public VulkanBufferObject<Block>
{
public:
    VulkanUBO(uint32_t nBlocks)
        : VulkanBufferObject<Block>(nBlocks)
    {
    }

    VkResult init(const VkPhysicalDeviceProperties &physicalDeviceProperties)
    {
        return VulkanBufferObject<Block>::init(physicalDeviceProperties.limits.minUniformBufferOffsetAlignment);
    }

    VkResult createBuffers(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t nBuffers)
    {
        return VulkanBufferObject<Block>::createBuffers(physicalDevice, device, nBuffers, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    }
};

}  // namespace vengine

#endif
